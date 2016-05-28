#include <stdlib.h>
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "string.h"
#include "at.h"
#include "driver/uart.h"
#include "driver/uart_register.h"
#include "user_interface.h"
#include "c_types.h"

extern uart_putchar_enable;

void ICACHE_FLASH_ATTR
FOO_at_command(uint8_t argc, char *argv[])
{
    int i;
    os_printf("%d\n", argc);
    for (i = 0; i <= argc; i++) {
        os_printf("%s\n", argv[i]);
    }
}

void ICACHE_FLASH_ATTR
AT_at_command(uint8_t argc, char *argv[])
{
    os_printf("OK\n");
}

void ICACHE_FLASH_ATTR
softap_config_print(struct softap_config *config)
{
    os_printf("ssid %s\npassword %s\nchannel %d\nauthmode %d\nbeacon_interval %d\n",
              config->ssid,
              config->password,
              config->channel,
              config->authmode,
              config->beacon_interval
    );
}
/**
 * AT+SOFTAP=<ssid>,<password>
 */
void ICACHE_FLASH_ATTR
SOFTAP_at_command(uint8_t argc, char *argv[])
{
    uint8_t mode, ret;
    size_t ssid_len = 0, pass_len = 0;
	struct softap_config config;

    os_bzero(&config, sizeof(struct softap_config));
    wifi_softap_get_config(&config);

    ssid_len = strlen(argv[0]);
    pass_len = strlen(argv[1]);

    if (ssid_len < 1 && ssid_len > 32) {
        os_printf("SSID length must be 1-32\n");
        return;
    }
    config.ssid_len = ssid_len;

    if (pass_len < 1 && pass_len > 64) {
        os_printf("Password length must be 1-64\n");
        return;
    }

    os_memcpy(&config.ssid, argv[0], 32);
    os_memcpy(&config.password, argv[1], 64);
    config.authmode = AUTH_WPA_PSK;
    config.channel  = 1;

    if ((mode = wifi_get_opmode()) != SOFTAP_MODE) {
        ETS_UART_INTR_DISABLE();
        wifi_set_opmode(SOFTAP_MODE);
        ETS_UART_INTR_ENABLE();
    }

    ETS_UART_INTR_DISABLE();
    ret = wifi_softap_set_config(&config);
    ETS_UART_INTR_ENABLE();

    if (ret) {
        softap_config_print(&config);
    } else {
        os_printf("error\n");
    }
}

os_timer_t wifi_check_timer;

void ICACHE_FLASH_ATTR
wifi_check_callback(void *arg)
{
    static uint8_t connections = 0;
    uint8_t status;
    os_timer_disarm(&wifi_check_timer);

    status = wifi_station_get_connect_status();

    switch (status = wifi_station_get_connect_status()) {
        case STATION_GOT_IP:
            os_printf("OK\n");
            return;
        case STATION_WRONG_PASSWORD:
            os_printf("WRONG PASSWORD\n");
            return;
        default:
            if (connections >= 3) {
                wifi_station_disconnect();
                return;
            }
    }

    os_timer_arm(&wifi_check_timer, 2000, 0);
}

/**
 * AT+STATION=<ssid>,<password>
 */

void ICACHE_FLASH_ATTR
STATION_at_command(uint8_t argc, char *argv[])
{
    char temp[64];
    struct station_config config;
    size_t ssid_len = 0, pass_len = 0;

    os_bzero(&config, sizeof(struct station_config));

    ssid_len = strlen(argv[0]);
    pass_len = strlen(argv[1]);

    if (ssid_len < 1 && ssid_len > 32) {
        os_printf("SSID length must be 1-32\n");
        return;
    }

    if (pass_len < 1 && pass_len > 64) {
        os_printf("Password length must be 1-64\n");
        return;
    }

    os_memcpy(&config.ssid, argv[0], 32);
    os_memcpy(&config.password, argv[1], 64);

    ETS_UART_INTR_DISABLE();
    wifi_station_set_config(&config);
    ETS_UART_INTR_ENABLE();
    wifi_station_connect();

    os_timer_setfn(&wifi_check_timer, (os_timer_func_t *)wifi_check_callback, NULL);
    os_timer_arm(&wifi_check_timer, 3000, 0);

}


void listap_callback(void *info, STATUS s)
{
    uint8 ssid[33];
    char temp[128];

    struct bss_info *link = (struct bss_info *)info;
    uart_putchar_enable = 1;

    if (s == OK) {
        link = link->next.stqe_next;
        for (; link != NULL; link = link->next.stqe_next) {
            os_memset(ssid, 0, 33);
            os_memcpy(ssid, link->ssid, os_strlen(link->ssid));
            os_sprintf(temp, "mac: \""MACSTR"\", auth: %d, channel: %d, ssdid: %s\n",
                       MAC2STR(link->bssid), link->authmode, link->channel, ssid);
            uart_putstr(temp);
        }
    }
}

/*
 * AT+LISTAP & AT
 */
void ICACHE_FLASH_ATTR
LISTAP_at_command(uint8_t argc, char *argv[])
{
    uart_putchar_enable = 0;
    if(wifi_station_scan(NULL, listap_callback)) {
//        uart_putstr("OK\n");
    } else {
        uart_putstr("error\n");
    }
}

#define AT_COMMANDS_MAX 5

at_command commands[AT_COMMANDS_MAX] = {
    AT_COMMAND(AT),
    AT_COMMAND(FOO),
    AT_COMMAND(SOFTAP),
    AT_COMMAND(LISTAP),
    AT_COMMAND(STATION)
};

void ICACHE_FLASH_ATTR
at_init(void)
{
    system_os_task(at_recv_task, AT_RECV_TASK, at_recv_queue,
            AT_RECV_QUEUE_LEN);
}

static void ICACHE_FLASH_ATTR
at_recv_task(os_event_t *event)
{
    uint8 ch;
    uint8 head[] = "AT";
    uint8 cmd[AT_CMD_MAXLEN];
    uint8 size = (READ_PERI_REG(UART_STATUS(UART0)) >> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT;
    int i, j, c, len = 0, valid = 1;

    if (size <= 0) {
        return;
    }

    uint8_t argc = 0;
    char *argv[AT_CMD_MAXARGS];

    for (i = 0; i < sizeof(head) - 1; i++) {
        ch = uart_getchar();
        if (ch != head[i]) {
            valid = 0;
        }
    }

    if (valid == 0) {
        while (ch = uart_getchar()){};
        os_printf("It is not valid AT command\n");
    }

    ch = uart_getchar();
    if (ch == '\n') {
        for (i = 0; i < AT_COMMANDS_MAX; i++) {
            at_command command = commands[i];
            if (os_memcmp(&head, command.command, sizeof(head)) == 0) {
                command.callback(argc, argv);
                break;
            }
        }
    }

    if (ch == '+') {
        for (ch = uart_getchar();
                ch != '\n' && ch != '=' && len < AT_CMD_MAXLEN; len++,
                ch = uart_getchar()) {
            cmd[len] = ch;
        }
    }

    if (len > 0) {
        cmd[++len] = '\0';
        for (i = 0; i < AT_COMMANDS_MAX; i++) {
            at_command command = commands[i];
            if (os_memcmp(&cmd, command.command, strlen(command.command)) == 0) {
                uint8 buff[2048];
                j = 0;
                c = 0;
                ch = uart_getchar();
                for (; c < 2048; c++, ch = uart_getchar()) {
                    if (ch == ' ')
                        continue;
                    if (ch == ',' || ch == '\n') {
                        buff[j] = 0;
                        j++;
                        argv[argc] = (char *) os_zalloc(sizeof(char) * j);
                        os_memcpy(argv[argc], &buff, j);
                        os_memset(&buff, 0, j);
                        j = 0;
                        argc++;
                        if (ch == '\n') {
                            break;
                        }
                    } else {
                        buff[j++] = ch;
                    }
                }

                command.callback(argc, argv);

                for (j = 0; j < argc; j++) {
                    os_free(argv[j]);
                }
                break;
            }
        }
    }

    if (UART_RXFIFO_FULL_INT_ST
            == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_FULL_INT_ST)) {
        WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR);
    } else if (UART_RXFIFO_TOUT_INT_ST
            == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_TOUT_INT_ST)) {
        WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_TOUT_INT_CLR);
    }

    ETS_UART_INTR_ENABLE();
}
