#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

/* 电机速度命令限制在 PWM 驱动的 0..10000 占空比范围内。 */
#define MOTOR_SPEED_MAX 10000

/* 速度闭环目标单位：每个 ENCODER_SAMPLE_MS 采样窗口内的编码器脉冲数。 */
#define MOTOR_TARGET_SPEED_MAX 80

/*
 * 左右轮速度 PI 参数。
 * 前馈项让 target=20 时大约输出 2000 PWM，接近原来的开环基础速度。
 * KP/KI 后续按实车情况微调：抖动就减小，跟不住就增大。
 */
#define MOTOR_SPEED_FF_NUM 100
#define MOTOR_SPEED_FF_DEN 1
#define MOTOR_SPEED_KP_NUM 80
#define MOTOR_SPEED_KP_DEN 1
#define MOTOR_SPEED_KI_NUM 4
#define MOTOR_SPEED_KI_DEN 1
#define MOTOR_SPEED_INTEGRAL_LIMIT 500

/* 初始化 PWM，并使能 TB6612。 */
void Motor_Init(void);

/* 停止两路电机，但不关闭 TB6612 的 STBY。 */
void Motor_Stop(void);

/* 控制 TB6612 的 STBY 引脚。 */
void Motor_Enable(void);
void Motor_Disable(void);

/* 有符号速度命令：正负决定方向，绝对值决定占空比。 */
void Motor_SetSpeed(int16_t leftSpeed, int16_t rightSpeed);
void Motor_SetSpeed_L(int16_t Speed);
void Motor_SetSpeed_R(int16_t Speed);

/* 设置左右轮目标速度，单位是一个采样周期内的编码器脉冲数。 */
void Motor_SetTargetSpeed(int16_t leftTarget, int16_t rightTarget);

/* Encoder_Update() 之后调用一次，根据编码器反馈计算实际 PWM 输出。 */
void Motor_SpeedControlUpdate(void);

/* 清空速度闭环目标和积分项，并停止电机。 */
void Motor_SpeedControlReset(void);

int16_t Motor_GetLeftTargetSpeed(void);
int16_t Motor_GetRightTargetSpeed(void);
int16_t Motor_GetLeftPwmOutput(void);
int16_t Motor_GetRightPwmOutput(void);

#endif
