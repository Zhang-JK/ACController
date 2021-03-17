#ifndef __timer_H__
#define __timer_H__

#include "stm32f1xx_hal.h"
#include "tim.h"

void timerInit();
void timerCallback(TIM_HandleTypeDef *htim);
extern uint64_t runningTime;

#endif
