#include "timer.h"

uint64_t runningTime = 0;

void timerInit() {
    HAL_TIM_RegisterCallback(&htim4, HAL_TIM_PERIOD_ELAPSED_CB_ID,
                           timerCallback);
    HAL_TIM_Base_Start_IT(&htim4);
}

void timerCallback(TIM_HandleTypeDef *htim) {
    runningTime++;
}