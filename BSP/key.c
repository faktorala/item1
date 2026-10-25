#include "key.h"

uint8_t get_press_id(void) {
    if(HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == 0)
        return 1;
    if(HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == 0)
        return 2;
    if(HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin) == 0)
        return 3;
    if(HAL_GPIO_ReadPin(KEY4_GPIO_Port, KEY4_Pin) == 0)
        return 4;
    return 0;
}