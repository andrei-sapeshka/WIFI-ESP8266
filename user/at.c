#include "os_type.h"
#include "osapi.h"
#include "at.h"
#include "driver/uart.h"
#include "driver/uart_register.h"

void ICACHE_FLASH_ATTR
at_init(void)
{
	system_os_task(at_recv_task, AT_RECV_TASK, at_recv_queue, AT_RECV_QUEUE_LEN);
}

static void ICACHE_FLASH_ATTR
at_recv_task(os_event_t *event)
{
	uint8 ch;
	ch = uart_getchar();

	os_printf("%x\n", ch);


	if (UART_RXFIFO_FULL_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_FULL_INT_ST)) {
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR);
	}else if (UART_RXFIFO_TOUT_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_TOUT_INT_ST)) {
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_TOUT_INT_CLR);
	}

	ETS_UART_INTR_ENABLE();
}
