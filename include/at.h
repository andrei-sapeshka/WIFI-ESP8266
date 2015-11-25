#ifndef __AT_H
#define __AT_H

#include "os_type.h"

//#define AT_COMMAND(NAME)  { #NAME, NAME ## _at_command }

#define AT_RECV_QUEUE_LEN 64
#define AT_RECV_TASK      0
#define AT_COMMANDS_MAX   2

os_event_t    at_recv_queue[AT_RECV_QUEUE_LEN];
//
//typedef struct
//{
//	uint8 *command;
//	void (*function) (uint8 *command);
//} at_command;
//
//void foo_at_command(uint8 *command);
//void bar_at_command(uint8 *command);
//
//at_command commands[AT_COMMANDS_MAX] = {
//		AT_COMMAND(foo),
//		AT_COMMAND(bar)
//};

void ICACHE_FLASH_ATTR at_init();

static void at_recv_task(os_event_t *event);

#endif
