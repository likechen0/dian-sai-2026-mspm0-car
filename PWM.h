#ifndef PWM_H
#define PWM_H

#include <stdint.h>

/* Software duty range: 10000 means 100.00%. */
#define PWM_DUTY_MAX 10000U

/* Start the timer and force both motor PWM channels to zero duty. */
void PWM_Init(void);

/* Set left/right motor PWM duty in the 0..PWM_DUTY_MAX range. */
void PWM_SetDuty_L(uint16_t duty);
void PWM_SetDuty_R(uint16_t duty);

#endif
