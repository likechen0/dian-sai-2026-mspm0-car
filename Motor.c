#include "Motor.h"
#include "PWM.h"
#include "ti_msp_dl_config.h"

/*
 * TB6612 motor driver.
 *
 * heyvhao pinout:
 * PWMA -> PA12, PWMB -> PA13
 * AIN1 -> PB17, AIN2 -> PB19
 * BIN1 -> PA16, BIN2 -> PB24
 *
 * left wheel command  -> A bridge: PWMA/AIN1/AIN2
 * right wheel command -> B bridge: PWMB/BIN1/BIN2
 * STBY is tied to +5V, so Motor_Enable/Disable do not control a GPIO pin.
 */

static int16_t Motor_ClampSpeed(int32_t speed)
{
    if (speed > (int32_t) PWM_DUTY_MAX) {
        return (int16_t) PWM_DUTY_MAX;
    }

    if (speed < -(int32_t) PWM_DUTY_MAX) {
        return -(int16_t) PWM_DUTY_MAX;
    }

    return (int16_t) speed;
}

void Motor_Init(void)
{
    PWM_Init();
    Motor_Enable();
    Motor_Stop();
}

void Motor_Enable(void)
{
    /* STBY is hardware-tied high on the heyvhao wiring. */
}

void Motor_Disable(void)
{
    Motor_Stop();
}

void Motor_Stop(void)
{
    PWM_SetDuty_L(0);
    PWM_SetDuty_R(0);

    DL_GPIO_clearPins(TB6612_PORTB_PORT,
        TB6612_PORTB_AIN1_PIN | TB6612_PORTB_AIN2_PIN |
        TB6612_PORTB_BIN2_PIN);
    DL_GPIO_clearPins(TB6612_PORTA_PORT, TB6612_PORTA_BIN1_PIN);
}

void Motor_SetSpeed(int16_t leftSpeed, int16_t rightSpeed)
{
    Motor_SetSpeed_L(leftSpeed);
    Motor_SetSpeed_R(rightSpeed);
}

void Motor_SetSpeed_L(int16_t Speed)
{
    Speed = Motor_ClampSpeed(Speed);

    if (Speed > 0) {
        DL_GPIO_clearPins(TB6612_PORTB_PORT, TB6612_PORTB_AIN1_PIN);
        DL_GPIO_setPins(TB6612_PORTB_PORT, TB6612_PORTB_AIN2_PIN);
        PWM_SetDuty_L((uint16_t) Speed);
    } else if (Speed < 0) {
        DL_GPIO_setPins(TB6612_PORTB_PORT, TB6612_PORTB_AIN1_PIN);
        DL_GPIO_clearPins(TB6612_PORTB_PORT, TB6612_PORTB_AIN2_PIN);
        PWM_SetDuty_L((uint16_t) -Speed);
    } else {
        PWM_SetDuty_L(0);
        DL_GPIO_clearPins(TB6612_PORTB_PORT,
            TB6612_PORTB_AIN1_PIN | TB6612_PORTB_AIN2_PIN);
    }
}

void Motor_SetSpeed_R(int16_t Speed)
{
    Speed = Motor_ClampSpeed(Speed);

    if (Speed > 0) {
        DL_GPIO_clearPins(TB6612_PORTA_PORT, TB6612_PORTA_BIN1_PIN);
        DL_GPIO_setPins(TB6612_PORTB_PORT, TB6612_PORTB_BIN2_PIN);
        PWM_SetDuty_R((uint16_t) Speed);
    } else if (Speed < 0) {
        DL_GPIO_setPins(TB6612_PORTA_PORT, TB6612_PORTA_BIN1_PIN);
        DL_GPIO_clearPins(TB6612_PORTB_PORT, TB6612_PORTB_BIN2_PIN);
        PWM_SetDuty_R((uint16_t) -Speed);
    } else {
        PWM_SetDuty_R(0);
        DL_GPIO_clearPins(TB6612_PORTA_PORT, TB6612_PORTA_BIN1_PIN);
        DL_GPIO_clearPins(TB6612_PORTB_PORT, TB6612_PORTB_BIN2_PIN);
    }
}
