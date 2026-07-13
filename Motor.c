#include "Motor.h"
#include "PWM.h"
#include "ti_msp_dl_config.h"

/*
 * lkc 扩展板上的 TB6612：
 *   逻辑左轮接 U16/R1/B 桥：PWMB PA7，BIN1 PB9，BIN2 PB6
 *   逻辑右轮接 U6/L1/A 桥：PWMA PB14，AIN1 PA13，AIN2 PB10
 *   STBY 在硬件上接 5V，不需要软件使能脚。
 * U7/L2 与 U6 同属 A 桥，U9/R2 与 U16 同属 B 桥，但都没有编码器线。
 * 原理图已经交叉连接 A 路 AO1/AO2 网络，此处不要再额外反转电机极性。
 */
static int16_t Motor_ClampSpeed(int32_t speed)
{
    if (speed > (int32_t)PWM_DUTY_MAX) {
        return (int16_t)PWM_DUTY_MAX;
    }
    if (speed < -(int32_t)PWM_DUTY_MAX) {
        return -(int16_t)PWM_DUTY_MAX;
    }
    return (int16_t)speed;
}

void Motor_Init(void)
{
    PWM_Init();
    Motor_Enable();
    Motor_Stop();
}

void Motor_Enable(void)
{
    /* STBY 已由扩展板硬件拉高。 */
}

void Motor_Disable(void)
{
    Motor_Stop();
}

void Motor_Stop(void)
{
    PWM_SetDuty_L(0);
    PWM_SetDuty_R(0);

    DL_GPIO_clearPins(TB6612_PORTA_PORT, TB6612_PORTA_AIN1_PIN);
    DL_GPIO_clearPins(TB6612_PORTB_PORT,
        TB6612_PORTB_AIN2_PIN |
        TB6612_PORTB_BIN1_PIN |
        TB6612_PORTB_BIN2_PIN);
}

void Motor_SetSpeed(int16_t leftSpeed, int16_t rightSpeed)
{
    Motor_SetSpeed_L(leftSpeed);
    Motor_SetSpeed_R(rightSpeed);
}

void Motor_SetSpeed_L(int16_t speed)
{
    speed = Motor_ClampSpeed(speed);

    if (speed > 0) {
        DL_GPIO_clearPins(TB6612_PORTB_PORT, TB6612_PORTB_BIN1_PIN);
        DL_GPIO_setPins(TB6612_PORTB_PORT, TB6612_PORTB_BIN2_PIN);
        PWM_SetDuty_L((uint16_t)speed);
    } else if (speed < 0) {
        DL_GPIO_setPins(TB6612_PORTB_PORT, TB6612_PORTB_BIN1_PIN);
        DL_GPIO_clearPins(TB6612_PORTB_PORT, TB6612_PORTB_BIN2_PIN);
        PWM_SetDuty_L((uint16_t)-speed);
    } else {
        PWM_SetDuty_L(0);
        DL_GPIO_clearPins(TB6612_PORTB_PORT,
            TB6612_PORTB_BIN1_PIN | TB6612_PORTB_BIN2_PIN);
    }
}

void Motor_SetSpeed_R(int16_t speed)
{
    speed = Motor_ClampSpeed(speed);

    if (speed > 0) {
        DL_GPIO_clearPins(TB6612_PORTA_PORT, TB6612_PORTA_AIN1_PIN);
        DL_GPIO_setPins(TB6612_PORTB_PORT, TB6612_PORTB_AIN2_PIN);
        PWM_SetDuty_R((uint16_t)speed);
    } else if (speed < 0) {
        DL_GPIO_setPins(TB6612_PORTA_PORT, TB6612_PORTA_AIN1_PIN);
        DL_GPIO_clearPins(TB6612_PORTB_PORT, TB6612_PORTB_AIN2_PIN);
        PWM_SetDuty_R((uint16_t)-speed);
    } else {
        PWM_SetDuty_R(0);
        DL_GPIO_clearPins(TB6612_PORTA_PORT, TB6612_PORTA_AIN1_PIN);
        DL_GPIO_clearPins(TB6612_PORTB_PORT, TB6612_PORTB_AIN2_PIN);
    }
}
