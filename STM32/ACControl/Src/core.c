#include "core.h"
#include <string.h>
#include "timer.h"
#include <stdio.h>

// int _write(int fd, char *ptr, int len) {
//   HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, 0xFFFF);
//   return len;
// }

int servo1_up = 1900;
int servo1_down = 1300;
int servo2_up = 1950;
int servo2_down = 1370;

struct State state;

char RxBuffer[RXBUFFERSIZE]; //接收数据
uint8_t aRxBuffer;           //接收中断缓冲
uint8_t Uart1_Rx_Cnt = 0;    //接收缓冲计数

int init_system() {
    state.sw = HAL_GPIO_ReadPin(MAIN_SWITCH_GPIO_Port, MAIN_SWITCH_Pin) == GPIO_PIN_RESET ? SYS_ON : SYS_OFF;
    state.re_sw = HAL_GPIO_ReadPin(REMOTE_SWITCH_GPIO_Port, REMOTE_SWITCH_Pin) == GPIO_PIN_RESET ? REMOTE_ON : REMOTE_OFF;
    state.mode = ONOFF_MODE;
    state.lightOff = 0;
    state.tarTemp = 26;
    state.tarTime[0] = 30;
    state.tarTime[1] = 30;
    state.startTime = 0;
    state.shutDownTime = 0;
    state.tempAchieveCounter = 0;

    // usDelayInit();
    timerInit();
    HAL_ADCEx_Calibration_Start(&hadc1);
    HAL_Delay(500);

    HAL_ADC_Start(&hadc1);
    HAL_UART_Receive_IT(&huart1, (uint8_t *)&aRxBuffer, 1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    if(state.sw == SYS_OFF) {
        __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, servo2_down);
        __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_1, servo1_down);
        HAL_GPIO_WritePin(LED_ON_BOARD_GPIO_Port, LED_ON_BOARD_Pin, GPIO_PIN_SET);

        while(HAL_GPIO_ReadPin(OFF_SWITCH_GPIO_Port, OFF_SWITCH_Pin) == GPIO_PIN_SET) {
            HAL_Delay(100);
        }
    }
    else {
        __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, servo2_up);
        __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_1, servo1_up);
        HAL_GPIO_WritePin(LED_ON_BOARD_GPIO_Port, LED_ON_BOARD_Pin, GPIO_PIN_RESET);

        while(HAL_GPIO_ReadPin(OFF_SWITCH_GPIO_Port, OFF_SWITCH_Pin) == GPIO_PIN_RESET) {
            HAL_Delay(100);
        }
    }

    HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, state.mode == ONOFF_MODE ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, state.mode == TIME_MODE  ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, state.mode == TEMP_MODE  ? GPIO_PIN_RESET : GPIO_PIN_SET);

    HAL_Delay(500);

    return 1;    
}

int string2int(char* array, int start, int end) {
	if(start > end) return 0;
    int result = 0;
    for(int i = end; i >= start; i--) {
        int num = ((int)array[i] - (int)'0');
        for(int j = end - i; j > 0; j--) num *= 10;
		// printf("%d\n", num);
        result += num;
    }
    return result;
}

void bluetooth_decode(char* msg) {
    int startInd = -1, endInd = -1;
    int count = 0;
    while((startInd == -1 || endInd == -1) && count < Uart1_Rx_Cnt) {
        if(startInd == -1) {if(RxBuffer[count] == '&') startInd = count;}
        else {if(RxBuffer[count] == '.') endInd = count;}
        count++;
    }

    if(startInd == -1 || endInd == -1) return ;
    else {
        int middleInd = 0;
        int input_temp = 0;
        switch(RxBuffer[startInd+1]) {
            case '0':
            if(RxBuffer[endInd - 1] == '2') {
                if(state.sw == SYS_ON) state.sw = SYS_OFF;
                else state.sw = SYS_ON;
                // strcpy(msg, "Change the system to ");
                // strcat(msg, state.sw == SYS_ON ? "ON\n" : "OFF\n");
            }
            break;
            case '1': 
            if(RxBuffer[endInd - 2] == ',') {
                if(RxBuffer[startInd + 3] == '0') state.mode = ONOFF_MODE;
                else if(RxBuffer[startInd + 3] == '1') state.mode = TIME_MODE;
                else if(RxBuffer[startInd + 3] == '2') state.mode = TEMP_MODE;
                if(RxBuffer[endInd - 1] == '0') state.lightOff = 0;
                else if(RxBuffer[endInd - 1] == '1') state.lightOff = 1;
            }
            break;
            case '2':
            
            for(int i = startInd; i < endInd; i++) 
                if(RxBuffer[i] == ',')
                    middleInd = i;
            if(middleInd != 0) {
                int on_time = string2int(RxBuffer, startInd + 3, middleInd - 1);
                int off_time = string2int(RxBuffer, middleInd + 1, endInd - 1);
                if(on_time >= 15 && off_time >= 15) {
                    state.tarTime[0] = on_time;
                    state.tarTime[1] = off_time;
                    printf("Changed the time to %d mins ON and %d mins OFF\n", on_time, off_time);
                }
            }
            break;
            case '3':
            input_temp = string2int(RxBuffer, startInd + 3, endInd - 1);
            if(input_temp <= 30 && input_temp >= 15) state.tarTemp = input_temp;
            printf("Changed the target temp to %3.1f\n", state.tarTemp);
            break;
        }
        Uart1_Rx_Cnt = 0;
        memset(RxBuffer, 0x00, sizeof(RxBuffer));
    }
}

void changeLight(enum Mode mode, int lightOff) {
    HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
    if(!lightOff) 
        if(mode == ONOFF_MODE)
            HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_RESET);
        else if(mode == TIME_MODE)
            HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
        else
            HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
}

char* msg[30];
void update_info() {
    strcpy(msg, "");
    msg[0] = '0';
    enum SystemSwitch last = state.sw;
    enum Mode lastMode = state.mode;
    state.re_sw = HAL_GPIO_ReadPin(REMOTE_SWITCH_GPIO_Port, REMOTE_SWITCH_Pin) == GPIO_PIN_RESET ? REMOTE_ON : REMOTE_OFF;
    if(state.re_sw != REMOTE_ON) {
        state.sw = HAL_GPIO_ReadPin(MAIN_SWITCH_GPIO_Port, MAIN_SWITCH_Pin) == GPIO_PIN_RESET ? SYS_ON : SYS_OFF;
        state.mode = ONOFF_MODE;
    }
    if(state.re_sw == REMOTE_ON && Uart1_Rx_Cnt != 0) bluetooth_decode(msg);
    state.on_sw = HAL_GPIO_ReadPin(ON_SWITCH_GPIO_Port, ON_SWITCH_Pin) == GPIO_PIN_RESET ? ON_OFF : ON_ON;
    state.off_sw = HAL_GPIO_ReadPin(OFF_SWITCH_GPIO_Port, OFF_SWITCH_Pin) == GPIO_PIN_RESET ? OFF_OFF : OFF_ON;
    state.temp = HAL_ADC_GetValue(&hadc1) * 0.0008056f * 100;
    if(last != state.sw) {
        move(state.sw);
        strcpy(msg, "Change the system to ");
        strcat(msg, state.sw == SYS_ON ? "ON\n" : "OFF\n");
    }
    if(lastMode != state.mode) {
        move(state.sw);
        strcpy(msg, "Change mode to ");
        strcat(msg, state.mode == ONOFF_MODE ? "ONOFF mode\n" : state.mode == TIME_MODE ? "TIME mode\n" : "TEMP mode\n");
        if(state.mode == TIME_MODE) {state.startTime = runningTime; state.shutDownTime = runningTime;}
        changeLight(state.mode, state.lightOff);
    }
    if(msg[0] != '0') printf(msg);
}

int lastAchievement = 0;
void send_state() {
    // char modeStr[15] = "ON/OFF Mode";
    char msg1[25];
    strcpy(msg1, "Now the air-con is ");
    strcat(msg1, state.sw == SYS_ON ? "ON\n" : "OFF\n");
    printf(msg1);

    char msg2[25];
    strcpy(msg2, "The current mode is ");
    strcat(msg2, state.mode == ONOFF_MODE ? "ON/OFF Mode\n" : state.mode == TEMP_MODE ? "Temp Mode\n" : "Time Mode\n");
    printf(msg2);

    printf("Current temperature is : %3.1f\n", state.temp);

    int currentTime = runningTime;
    char msg3[25];
    strcpy(msg3, "The air-con has ");
    strcat(msg3, state.sw == SYS_ON ? "run" : "closed");
    strcat(msg3, " for ");
    printf("%s%d%s", msg3, (currentTime - (state.sw == SYS_ON ? state.startTime : state.shutDownTime)) / 60, " minutes\n");

    if(state.mode == TEMP_MODE){
        printf("The target temperature is %3.1f\n", state.tarTemp);
        printf("The last temperature achievement is %d\n", lastAchievement);
    }
    else if(state.mode == TIME_MODE) {
        int passTime = (currentTime - (state.sw == SYS_ON ? state.startTime : state.shutDownTime)) / 60;
        int remainingTime = (state.sw == SYS_ON ? state.tarTime[0] : state.tarTime[1]) - passTime;
        printf("There are %d minutes before air-con %s\n", remainingTime, state.sw == SYS_ON ? "close" : "start");
        printf("Open for %d mins and close for %d mins\n", state.tarTime[0], state.tarTime[1]);
    }

    printf("\n");
}

void move(enum SystemSwitch pos) {
    if(pos == SYS_ON) {
        HAL_Delay(100);
        __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, servo2_up);
        HAL_Delay(2000);
        __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_1, servo1_up);
        HAL_Delay(100);

        while(HAL_GPIO_ReadPin(OFF_SWITCH_GPIO_Port, OFF_SWITCH_Pin) == GPIO_PIN_RESET) {
            HAL_Delay(100);
        }

        state.startTime = runningTime;
        HAL_GPIO_WritePin(LED_ON_BOARD_GPIO_Port, LED_ON_BOARD_Pin, GPIO_PIN_RESET);
    }

    else {
        HAL_Delay(100);
        __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_1, servo1_down);
        HAL_Delay(2000);
        __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, servo2_down);
        HAL_Delay(100);

        while(HAL_GPIO_ReadPin(OFF_SWITCH_GPIO_Port, OFF_SWITCH_Pin) == GPIO_PIN_SET) {
            HAL_Delay(100);
        }

        state.shutDownTime = runningTime;
        HAL_GPIO_WritePin(LED_ON_BOARD_GPIO_Port, LED_ON_BOARD_Pin, GPIO_PIN_SET);
    }
}

int checkounter = 0;
void tempCheck() {
    if(state.sw == SYS_ON)
        if(state.temp < state.tarTemp - 0.5)
            state.tempAchieveCounter++;
    if(state.sw == SYS_OFF)
        if(state.temp > state.tarTemp + 0.5)
            state.tempAchieveCounter++;
    if(checkounter >= 60) {
        if(state.tempAchieveCounter >= 45)
            if(state.sw == SYS_ON) state.sw = SYS_OFF;
            else state.sw = SYS_ON;
        move(state.sw);
        checkounter = 0;
        lastAchievement = state.tempAchieveCounter;
        state.tempAchieveCounter = 0;
    }
    else checkounter++;
}

void timeCheck() {
    if(state.sw == SYS_ON){
        if(runningTime - state.startTime >= state.tarTime[0]*60) {
            state.sw = SYS_OFF;
            move(state.sw);
        }
    }
    else {
        if(runningTime - state.shutDownTime >= state.tarTime[1]*60) {
            state.sw = SYS_ON;
            move(state.sw);
        }
    }

}

