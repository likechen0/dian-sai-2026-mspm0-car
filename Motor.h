#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

/* 电机速度命令限制在 PWM 驱动的 0..10000 占空比范围内。 */
#define MOTOR_SPEED_MAX 10000

/* 初始化 PWM，并使能 TB6612。 */
void Motor_Init(void);

/* 停止两路电机，但不关闭 TB6612 的 STBY。 */
void Motor_Stop(void);

/* lkc 扩展板上 STBY 硬接 +5V，这两个函数只保留接口兼容。 */
void Motor_Enable(void);
void Motor_Disable(void);

/* 有符号速度命令：正负决定方向，绝对值决定占空比。 */
void Motor_SetSpeed(int16_t leftSpeed, int16_t rightSpeed);
void Motor_SetSpeed_L(int16_t Speed);
void Motor_SetSpeed_R(int16_t Speed);

#endif
