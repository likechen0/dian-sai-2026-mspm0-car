#ifndef TRACKING_H
#define TRACKING_H

#include <stdint.h>

/*
 * 龙邱 8 路灰度模块 SC 口接线：
 * AS -> PA25(ADC0_CH2), S0 -> PA24, S1 -> PA26, S2 -> PA27.
 */
#define TRACKING_VALUE_MAX       100U
#define TRACKING_CHANNEL_COUNT   8U

/* 如果黑线 ADC 值比白底小，就把这个宏改成 0。 */
#define TRACKING_BLACK_IS_HIGH   1

/* 归一化值超过该阈值，就认为这一通道检测到线。 */
#define TRACKING_LINE_THRESHOLD  30U

/* 巡线基础 PWM 和 PD 参数。 */
#define TRACKING_BASE_SPEED      2000
#define TRACKING_KP_NUM          43
#define TRACKING_KP_DEN          100
#define TRACKING_KD_NUM          5
#define TRACKING_KD_DEN          10

/* 最近一次 8 路归一化灰度值，范围 0..TRACKING_VALUE_MAX。 */
extern unsigned char LQ_Tracking_Value[TRACKING_CHANNEL_COUNT];

/* 加权循迹误差：负数表示线偏左，正数表示线偏右。 */
extern volatile int16_t Tracking_Error;

/* 最近一次 PD 修正量，会加到一侧轮子、从另一侧轮子减掉。 */
extern volatile int16_t Tracking_Correction;

/* 至少有一路超过阈值时为非 0，表示当前检测到线。 */
extern volatile uint8_t Tracking_LineDetected;

/* 初始化 ADC 和通道选择 GPIO。 */
void Tracking_Init(void);
void Tracking_Adc_Init(void);

/* 通过 S2/S1/S0 选择一路传感器。 */
void Tracking_IO_Set(unsigned char s2, unsigned char s1, unsigned char s0);

/* 读取当前已选择通道的一次 ADC 值。 */
uint16_t Tracking_Adc_once(void);

/* 选择并读取一路传感器，ch 从 1 开始，内部做简单平均。 */
unsigned int Tracking_Value_once(unsigned char ch);

/* 刷新 8 路灰度值。 */
void Tracking_Value_Acquire(void);

/* 计算是否有线，以及加权循迹误差。 */
int16_t Tracking_CalcError(void);
uint8_t Tracking_IsSensorOnLine(uint8_t channel);
uint8_t Tracking_GetLineMask(void);

/* 独立巡线测试函数；当前主导航状态机不直接使用它。 */
void Tracking_LineFollowStep(void);

#endif
