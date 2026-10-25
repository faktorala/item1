#include "schedule.h"

static sched_t sched_manager[] = {
    {.task = NULL, .argument = NULL, .cnt = 0, .period = 0},
};

void sched_poll(void) {
    for (uint8_t i = 0; i < sizeof(sched_manager)/sizeof(sched_manager[0]); i ++) {
        if(++sched_manager[i].cnt >= sched_manager[i].period) {
            sched_manager[i].cnt = 0;
            if(sched_manager[i].task != NULL) {
                sched_manager[i].task(sched_manager[i].argument);
            }
        }

    }
}

void user_poll(void) {
    
}
