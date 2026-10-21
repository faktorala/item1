#ifndef __ISR_H
#define __ISR_H

#include <stdint.h>
#include "spi.h"
#include "tim.h"
#include "usart.h"

uint8_t IsrUserInit(void);
void UserEvPoll(void);
#endif
