#include "Motor.h"
#include "PWM.h"
#include "ti_msp_dl_config.h"

/*
 * TB6612 电机驱动模块。
 * PWM 控制速度，AIN1/AIN2 和 BIN1/BIN2 控制方向。
 * 速度为正表示当前接线下定义的“前进”方向。
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
    /* 先打开 TB6612 的 STBY，再主动停止两路 H 桥。 */
    PWM_Init();
    Motor_Enable();
    Motor_Stop();
}

void Motor_Enable(void)
{
    DL_GPIO_setPins(TB6612_PORTB_PORT, TB6612_PORTB_STBY_PIN);
}

void Motor_Disable(void)
{
    Motor_Stop();
    DL_GPIO_clearPins(TB6612_PORTB_PORT, TB6612_PORTB_STBY_PIN);
}

void Motor_Stop(void)
{
    /* 去掉 PWM，并清空方向脚，让两路电机停止。 */
    PWM_SetDuty_L(0);
    PWM_SetDuty_R(0);
    DL_GPIO_clearPins(TB6612_PORTA_PORT,
        TB6612_PORTA_AIN1_PIN | TB6612_PORTA_BIN1_PIN | TB6612_PORTA_BIN2_PIN);
    DL_GPIO_clearPins(TB6612_PORTB_PORT, TB6612_PORTB_AIN2_PIN);
}

void Motor_SetSpeed(int16_t leftSpeed, int16_t rightSpeed)
{
    Motor_SetSpeed_L(leftSpeed);
    Motor_SetSpeed_R(rightSpeed);
}

void Motor_SetSpeed_L(int16_t Speed)
{
    Speed = Motor_ClampSpeed(Speed);

    /* 左电机：正负速度通过交换 AIN1/AIN2 实现正反转。 */
    if (Speed > 0) {
        DL_GPIO_setPins(TB6612_PORTA_PORT, TB6612_PORTA_AIN1_PIN);
        DL_GPIO_clearPins(TB6612_PORTB_PORT, TB6612_PORTB_AIN2_PIN);
        PWM_SetDuty_L((uint16_t) Speed);
    } else if (Speed < 0) {
        DL_GPIO_clearPins(TB6612_PORTA_PORT, TB6612_PORTA_AIN1_PIN);
        DL_GPIO_setPins(TB6612_PORTB_PORT, TB6612_PORTB_AIN2_PIN);
        PWM_SetDuty_L((uint16_t) -Speed);
    } else {
        PWM_SetDuty_L(0);
        DL_GPIO_clearPins(TB6612_PORTA_PORT, TB6612_PORTA_AIN1_PIN);
        DL_GPIO_clearPins(TB6612_PORTB_PORT, TB6612_PORTB_AIN2_PIN);
    }
}

void Motor_SetSpeed_R(int16_t Speed)
{
    Speed = Motor_ClampSpeed(Speed);

    /* 右电机：正负速度通过交换 BIN1/BIN2 实现正反转。 */
    if (Speed > 0) {
        DL_GPIO_setPins(TB6612_PORTA_PORT, TB6612_PORTA_BIN1_PIN);
        DL_GPIO_clearPins(TB6612_PORTA_PORT, TB6612_PORTA_BIN2_PIN);
        PWM_SetDuty_R((uint16_t) Speed);
    } else if (Speed < 0) {
        DL_GPIO_clearPins(TB6612_PORTA_PORT, TB6612_PORTA_BIN1_PIN);
        DL_GPIO_setPins(TB6612_PORTA_PORT, TB6612_PORTA_BIN2_PIN);
        PWM_SetDuty_R((uint16_t) -Speed);
    } else {
        PWM_SetDuty_R(0);
        DL_GPIO_clearPins(TB6612_PORTA_PORT,
            TB6612_PORTA_BIN1_PIN | TB6612_PORTA_BIN2_PIN);
    }
}
