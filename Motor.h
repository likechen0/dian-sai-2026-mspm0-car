#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

#define MOTOR_SPEED_MAX 10000

void Motor_Init(void);
void Motor_Stop(void);
void Motor_Enable(void);
void Motor_Disable(void);
void Motor_SetSpeed(int16_t leftSpeed, int16_t rightSpeed);
void Motor_SetSpeed_L(int16_t Speed);
void Motor_SetSpeed_R(int16_t Speed);

#endif
