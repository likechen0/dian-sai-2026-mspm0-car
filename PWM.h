#ifndef PWM_H
#define PWM_H

#include <stdint.h>

#define PWM_DUTY_MAX 10000U

void PWM_Init(void);
void PWM_SetDuty_L(uint16_t duty);
void PWM_SetDuty_R(uint16_t duty);

#endif
