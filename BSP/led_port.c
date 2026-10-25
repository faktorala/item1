#include "led_port.h"

void led_toggle(void) {
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
}

void led_set_value(GPIO_PinState PinState) {
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, PinState);
}
