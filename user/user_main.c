#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "driver/uart.h"

static volatile os_timer_t timer;

void user_rf_pre_init(void) { return; }

void timer_callback(void *arg)
{
	  os_printf("\r\nready!!!\r\n");
	  uart0_sendStr("\r\nready\r\n");
}

void ICACHE_FLASH_ATTR
user_init()
{
	uart_init(BIT_RATE_115200);
//    os_timer_disarm(&timer);
//    os_timer_setfn(&timer, (os_timer_func_t *)timer_callback, NULL);
//    os_timer_arm(&timer, 1000, 1);
//    system_os_task(task, 0, taskQueue, 1);
}
