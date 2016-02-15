#include <stdlib.h>
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "string.h"
#include "at.h"
#include "driver/uart.h"
#include "driver/uart_register.h"
#include "user_interface.h"

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

/**
 * AT+SOFTAP=<ssid>
 */
void ICACHE_FLASH_ATTR
SOFTAP_at_command(uint8_t argc, char *argv[])
{
    uint8_t mode, ret;
    size_t len = 0;
	struct softap_config config;
    mode = wifi_get_opmode();

    os_printf("%s\n", argv[0]);
    os_printf("%d\n", mode);

	os_bzero(&config, sizeof(struct softap_config));

	if (wifi_softap_get_config(&config) == 0) {
		os_printf("wifi_softap_get_config: ERROR\n");
		return;
	}


    len = strlen(argv[0]);
    if (len > 1 && len < 64) {
        os_memcpy(&config.ssid, argv[0], len);
    }

    if ((mode = wifi_get_opmode()) != SOFTAP_MODE) {
        ETS_UART_INTR_DISABLE();
        wifi_set_opmode(SOFTAP_MODE);
        ETS_UART_INTR_ENABLE();
    }

    ETS_UART_INTR_DISABLE();
    wifi_softap_set_config(&config);
    ETS_UART_INTR_ENABLE();

    os_printf("ssid %s\npassword %s\nchannel %d\nauthmode %d\nbeacon_interval %d\n",
              config.ssid,
              config.password,
              config.channel,
              config.authmode,
              config.beacon_interval
    );
}

#define AT_COMMANDS_MAX 3

at_command commands[AT_COMMANDS_MAX] = {
    AT_COMMAND(AT),
    AT_COMMAND(FOO),
    AT_COMMAND(SOFTAP)
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
