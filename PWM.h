#ifndef PWM_H
#define PWM_H

#include <stdint.h>

/* 软件占空比范围：10000 表示 100.00%。 */
#define PWM_DUTY_MAX 10000U

/* 启动 PWM 定时器，并把两路电机占空比置零。 */
void PWM_Init(void);

/* 设置左右电机 PWM，占空比范围为 0..PWM_DUTY_MAX。 */
void PWM_SetDuty_L(uint16_t duty);
void PWM_SetDuty_R(uint16_t duty);

#endif
