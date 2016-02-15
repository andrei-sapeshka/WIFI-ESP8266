#ifndef __AT_H
#define __AT_H

#include "os_type.h"

#define AT_RECV_QUEUE_LEN 64
#define AT_RECV_TASK      0

os_event_t    at_recv_queue[AT_RECV_QUEUE_LEN];

#define AT_COMMAND(NAME)  { #NAME, NAME ##_at_command }

#define AT_CMD_MAXLEN 126

#define AT_CMD_MAXARGS 5

#define AT_DATA_MAXLEN 832

typedef struct
{
    char *command;
    void (*callback) (uint8_t argc, char *argv[]);
} at_command;

void ICACHE_FLASH_ATTR at_init();

static void at_recv_task(os_event_t *event);

#endif
