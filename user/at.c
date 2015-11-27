#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "at.h"
#include "driver/uart.h"
#include "driver/uart_register.h"

void ICACHE_FLASH_ATTR
FOO_at_command(uint8_t argc, char *argv[])
{
    int i;
    for (i = 0; i < argc; i++) {
        os_printf("%s\n", argv[i]);
    }
}

void ICACHE_FLASH_ATTR
AT_at_command(uint8_t argc, char *argv[])
{
    int i;
    for (i = 0; i < argc; i++) {
    	os_printf("%s\n", argv[i]);
    }
}

#define AT_COMMANDS_MAX 2

at_command commands[AT_COMMANDS_MAX] = {
		AT_COMMAND(AT),
        AT_COMMAND(FOO)
};

void ICACHE_FLASH_ATTR
at_init(void)
{
	system_os_task(at_recv_task, AT_RECV_TASK, at_recv_queue, AT_RECV_QUEUE_LEN);
}

static void ICACHE_FLASH_ATTR
at_recv_task(os_event_t *event)
{
	uint8 ch;
	uint8 head[] = "AT";
	uint8 cmd[128];
	int i, c, len = 0, valid = 1;

	uint8_t argc = 0;
	char *argv[AT_CMD_MAXARGS];

	for (i = 0; i < sizeof(head)-1; i++) {
		ch = uart_getchar();
		if (ch != head[i]) {
			valid = 0;
		}
	}

	if (valid == 0) {
		while(ch = uart_getchar());
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
             ch != '\n' && ch != '=' && len < AT_CMD_MAXLEN;
             len++, ch = uart_getchar()) {
             cmd[len] = ch;
        }
    }

    if (len > 0) {
		cmd[++len] = '\0';
		for (i = 0; i < AT_COMMANDS_MAX; i++) {
			at_command command = commands[i];
			if (os_memcmp(&cmd, command.command, strlen(command.command)) == 0) {
				uint8 buff[2048];
				i = 0;
				c = 0;
				ch = uart_getchar();
				for (; c < 2048; c++, ch = uart_getchar()) {
					if (ch == ' ')
						continue;
					if (ch == ',' || ch == '\n') {
						buff[i] = 0; i++;
						argv[argc] = (char *)os_zalloc(sizeof(char) * i);
						os_memcpy(argv[argc], &buff, i);
						os_memset(&buff, 0, i);
						i = 0;
						argc++;
						if (ch == '\n') {
							break;
						}
					} else {
						buff[i++] = ch;
					}
				}

				command.callback(argc, argv);

				for (i = 0; i < AT_CMD_MAXARGS; i++) {
//					os_free(argv[i]);
				}
				break;
			}
		}
	}

	if (UART_RXFIFO_FULL_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_FULL_INT_ST)) {
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR);
	}else if (UART_RXFIFO_TOUT_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_TOUT_INT_ST)) {
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_TOUT_INT_CLR);
	}

	ETS_UART_INTR_ENABLE();
}
