#include "ISR.h"

uint8_t IsrUserInit(void) 
{
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
    __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_2, 500);
    HAL_TIM_Base_Start(&htim1);
    HAL_TIM_Base_Start_IT(&htim2);

    HAL_UART_AbortTransmit_IT(&huart1);
  
}

void UserEvPoll(void) {
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    HAL_UART_Transmit_DMA(&huart1, "hello\n", 7);
}
