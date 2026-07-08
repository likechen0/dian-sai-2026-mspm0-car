#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

/* Keep motor commands inside the PWM driver's 0..10000 duty range. */
#define MOTOR_SPEED_MAX 10000

/* Initialize PWM and TB6612 standby state. */
void Motor_Init(void);

/* Stop both motors without disabling the TB6612 standby pin. */
void Motor_Stop(void);

/* Control the TB6612 STBY input. */
void Motor_Enable(void);
void Motor_Disable(void);

/* Signed speed command: positive/negative choose direction, magnitude is duty. */
void Motor_SetSpeed(int16_t leftSpeed, int16_t rightSpeed);
void Motor_SetSpeed_L(int16_t Speed);
void Motor_SetSpeed_R(int16_t Speed);

#endif
