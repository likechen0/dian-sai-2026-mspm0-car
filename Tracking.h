#ifndef TRACKING_H
#define TRACKING_H

#include <stdint.h>

/*
 * lkc 扩展板把感为八路模拟量分别接到两个 ADC：
 * S1 PB19/ADC1_CH6，S2 PB17/ADC1_CH4，S3 PA16/ADC1_CH1
 * S4 PA14/ADC0_CH12，S5 PB20/ADC0_CH6，S6 PB25/ADC0_CH4
 * S7 PA25/ADC0_CH2，S8 PA27/ADC0_CH0
 */
#define TRACKING_CHANNEL_COUNT       8U

/* 归一化后 0 表示黑，4096 表示白。 */
#define TRACKING_NORMAL_MAX          4096U
#define TRACKING_VALUE_MAX           100U

/*
 * 黑线强度超过该值才算看到线。
 * 白底误判为 1 时增大，黑线识别不到时减小。
 */
#define TRACKING_LINE_THRESHOLD      1000U

/*
 * H10 插座从 OUT_1 到 OUT_8 的物理顺序如果与车头左右相反，
 * 只需把该宏改为 1，不要交换八组标定值。
 */
#define TRACKING_REVERSE_ORDER       0U

/*
 * PD 调参入口：
 * Kp = TRACKING_KP_NUM / TRACKING_KP_DEN
 * Kd = TRACKING_KD_NUM / TRACKING_KD_DEN
 * 误差范围大约为 -3500..3500。
 */
#define TRACKING_BASE_SPEED          2000
#define TRACKING_KP_NUM              40
#define TRACKING_KP_DEN              100
#define TRACKING_KD_NUM              10
#define TRACKING_KD_DEN              10

extern uint16_t Ganv_Tracking_Raw[TRACKING_CHANNEL_COUNT];
extern uint16_t Ganv_Tracking_Normal[TRACKING_CHANNEL_COUNT];
extern volatile uint8_t Ganv_Tracking_LineMask;
extern volatile uint8_t Ganv_Tracking_WhiteMask;

/* 为兼容显示和导航代码保留的 0..100 黑线强度数组。 */
extern unsigned char LQ_Tracking_Value[TRACKING_CHANNEL_COUNT];

/* 负数表示黑线偏左，正数表示黑线偏右。 */
extern volatile int16_t Tracking_Error;
extern volatile int16_t Tracking_Correction;
extern volatile uint8_t Tracking_LineDetected;

void Tracking_Init(void);
void Tracking_Adc_Init(void);
unsigned int Tracking_Value_once(unsigned char channel);
void Tracking_Value_Acquire(void);

int16_t Tracking_CalcError(void);
uint8_t Tracking_IsSensorOnLine(uint8_t channel);
uint8_t Tracking_GetLineMask(void);
uint16_t Tracking_GetLineStrength(uint8_t channel);

/* 独立测试函数；主程序正式运行时由 InertialNav.c 的状态机控制。 */
void Tracking_LineFollowStep(void);

#endif
