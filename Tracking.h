#ifndef TRACKING_H
#define TRACKING_H

#include <stdint.h>

/*
 * 感为无 MCU 八路灰度传感器接线：
 * OUT -> PA18(ADC1_CH3)
 * AD0 -> PB25, AD1 -> PB18, AD2 -> PB21
 * EN  -> 悬空或接 GND 都是使能，不要接高电平
 */
#define TRACKING_CHANNEL_COUNT       8U

/*
 * 归一化范围沿用感为例程的 12 位量程。
 * Ganv_Tracking_Normal: 0 表示黑，4096 表示白。
 * LQ_Tracking_Value:    0..100 的黑线强度，数值越大越像黑线。
 */
#define TRACKING_NORMAL_MAX          4096U
#define TRACKING_VALUE_MAX           100U

/*
 * 白底黑线识别阈值：
 * 黑线强度超过该值才认为该路看到线。
 * 如果 OLED 的 S: 一直全 0，优先减小这个值或重新测黑白标定。
 * 如果白地也误判为 1，优先增大这个值。
 */
#define TRACKING_LINE_THRESHOLD      1000U

/*
 * 如果 AD0/AD1/AD2 的实际地址逻辑和手册相反，把它改成 1。
 * 如果从左到右的通道顺序反了，把 TRACKING_REVERSE_ORDER 改成 1。
 */
#define TRACKING_ADDR_INVERT         0U
#define TRACKING_REVERSE_ORDER       0U

/*
 * PID 调参入口：
 * - TRACKING_BASE_SPEED 是普通循迹基础速度。
 * - Kp = TRACKING_KP_NUM / TRACKING_KP_DEN，负责把车拉回黑线。
 * - Ki = TRACKING_KI_NUM / TRACKING_KI_DEN，每个控制周期累加一次，
 *   用于补偿圆环等持续曲率造成的稳态偏差。
 * - Kd = TRACKING_KD_NUM / TRACKING_KD_DEN，负责抑制来回摆动。
 * 当前版本是感为传感器调车版，PID 还需要按实车速度、轮距、传感器高度继续调。
 * 误差范围大约为 -3500..3500。
 */
#define TRACKING_BASE_SPEED          2000
#define TRACKING_KP_NUM              21
#define TRACKING_KP_DEN              100
#define TRACKING_KI_NUM              1
#define TRACKING_KI_DEN              100
#define TRACKING_KD_NUM              12
#define TRACKING_KD_DEN              10

/*
 * 条件积分与抗饱和：
 * - 误差绝对值超过 ACTIVE_ERROR 时由 P/D 负责，I 项清零。
 * - 误差落在 DEADBAND 内时保持现有 I 项，不继续积累传感器噪声。
 * - I 项输出最多贡献 +/-600，避免长时间圆环后发生积分饱和。
 */
#define TRACKING_I_ACTIVE_ERROR      1200
#define TRACKING_I_DEADBAND          40
#define TRACKING_I_ERROR_LIMIT       60000L
#define TRACKING_I_OUTPUT_LIMIT      600

extern uint16_t Ganv_Tracking_Raw[TRACKING_CHANNEL_COUNT];
extern uint16_t Ganv_Tracking_Normal[TRACKING_CHANNEL_COUNT];
extern volatile uint8_t Ganv_Tracking_LineMask;
extern volatile uint8_t Ganv_Tracking_WhiteMask;

/* 为了兼容原工程显示/调试代码，保留这个旧名字。 */
extern unsigned char LQ_Tracking_Value[TRACKING_CHANNEL_COUNT];

/* 负数表示黑线偏左，正数表示黑线偏右。 */
extern volatile int16_t Tracking_Error;

/* 最近一次差速修正量：左轮加 correction，右轮减 correction。 */
extern volatile int16_t Tracking_Correction;

/* 当前 PID 中 I 项单独贡献的差速修正量，便于调试观察。 */
extern volatile int16_t Tracking_IntegralCorrection;

/* 至少一路超过阈值时为 1。 */
extern volatile uint8_t Tracking_LineDetected;

void Tracking_Init(void);
void Tracking_Adc_Init(void);

void Tracking_IO_Set(unsigned char ad2, unsigned char ad1, unsigned char ad0);
uint16_t Tracking_Adc_once(void);
unsigned int Tracking_Value_once(unsigned char ch);
void Tracking_Value_Acquire(void);

int16_t Tracking_CalcError(void);
uint8_t Tracking_IsSensorOnLine(uint8_t channel);
uint8_t Tracking_GetLineMask(void);
uint16_t Tracking_GetLineStrength(uint8_t channel);

/* 导航状态切换时清除积分；正常循迹时每个控制周期调用一次更新积分。 */
void Tracking_ResetIntegral(void);
int16_t Tracking_UpdateIntegral(int16_t error);

/* 独立测试用函数，主程序实际走 InertialNav.c 的状态机。 */
void Tracking_LineFollowStep(void);

#endif
