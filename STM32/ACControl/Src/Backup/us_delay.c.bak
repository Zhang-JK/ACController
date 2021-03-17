#include "us_delay.h"
#include "stm32f1xx_hal_tim.h"

int us_delay_timeout = 0;
static volatile int counterD = 0;
// static volatile int counterCounter = 0;
void timeoutCallback(TIM_HandleTypeDef *htim);

void usDelayInit() {
  HAL_TIM_RegisterCallback(&htim3, HAL_TIM_PERIOD_ELAPSED_CB_ID,
                           timeoutCallback);
}

void timeoutCallback(TIM_HandleTypeDef *htim) {
  us_delay_timeout = 1;
  counterD++;
}

void bsp_delay_us(uint16_t us) {
  uint16_t differ = 0xffff - us;

  us_delay_timeout = 0;
  __HAL_TIM_SET_COUNTER(&htim3, differ);
  HAL_TIM_Base_Start_IT(&htim3);

  while (us_delay_timeout == 0)
    ;
  HAL_TIM_Base_Stop(&htim3);
}
