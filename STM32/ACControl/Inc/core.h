#ifndef __core_H
#define __core_H
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

extern int servo1_up;
extern int servo1_down;
extern int servo2_up;
extern int servo2_down;

extern char RxBuffer[RXBUFFERSIZE]; //接收数据
extern uint8_t aRxBuffer;           //接收中断缓冲
extern uint8_t Uart1_Rx_Cnt;    //接收缓冲计数

enum SystemSwitch {
    SYS_OFF = 0,
    SYS_ON = 1
};

enum OnSwitch {
    ON_OFF = 0,
    ON_ON = 1
};

enum OffSwitch {
    OFF_OFF = 0,
    OFF_ON = 1
};

enum RemoteControl {
    REMOTE_ON = 0,
    REMOTE_OFF = 1
};

enum Mode {
    ONOFF_MODE = 0,
    TIME_MODE = 1,
    TEMP_MODE = 2
};

struct State {
    enum SystemSwitch sw;
    enum OnSwitch on_sw;
    enum OffSwitch off_sw;
    enum RemoteControl re_sw;
    enum Mode mode;
    float tarTemp;
    int tarTime[2];
    int lightOff;
    float temp;
    int startTime;
    int shutDownTime;
    int tempAchieveCounter;
};

extern struct State state;

int init_system();
void update_info();
void send_state();
void tempCheck();
void timeCheck();

#endif