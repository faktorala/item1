#ifndef __SCHEDULE_H
#define __SCHEDULE_H

#include <stdint.h>
#include "key.h"
#include "led_port.h"

typedef struct {
    void (*task)(void *argument);
    void *argument;
    uint32_t cnt;
    uint32_t period;
}sched_t;

void user_poll (void);

#endif
