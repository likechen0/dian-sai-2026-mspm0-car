#include "Motor.h"
#include "Encoder.h"
#include "PWM.h"
#include "ti_msp_dl_config.h"

/*
 * TB6612 电机驱动模块。
 * PWM 控制速度，AIN1/AIN2 和 BIN1/BIN2 控制方向。
 * 速度为正表示当前接线下定义的“前进”方向。
 */
static int16_t gLeftTargetSpeed;
static int16_t gRightTargetSpeed;
static int32_t gLeftIntegral;
static int32_t gRightIntegral;
static int16_t gLeftPwmOutput;
static int16_t gRightPwmOutput;

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

static int16_t Motor_ClampTargetSpeed(int32_t speed)
{
    if (speed > (int32_t) MOTOR_TARGET_SPEED_MAX) {
        return (int16_t) MOTOR_TARGET_SPEED_MAX;
    }

    if (speed < -(int32_t) MOTOR_TARGET_SPEED_MAX) {
        return -(int16_t) MOTOR_TARGET_SPEED_MAX;
    }

    return (int16_t) speed;
}

static int32_t Motor_ClampIntegral(int32_t integral)
{
    if (integral > MOTOR_SPEED_INTEGRAL_LIMIT) {
        return MOTOR_SPEED_INTEGRAL_LIMIT;
    }
    if (integral < -MOTOR_SPEED_INTEGRAL_LIMIT) {
        return -MOTOR_SPEED_INTEGRAL_LIMIT;
    }
    return integral;
}

static int16_t Motor_CalcPwmBySpeedPI(int16_t target, int16_t measured,
                                      int32_t *integral)
{
    int32_t error;
    int32_t pwm;

    if (target == 0) {
        *integral = 0;
        return 0;
    }

    error = (int32_t) target - measured;
    *integral = Motor_ClampIntegral(*integral + error);

    pwm = ((int32_t) target * MOTOR_SPEED_FF_NUM) / MOTOR_SPEED_FF_DEN;
    pwm += (error * MOTOR_SPEED_KP_NUM) / MOTOR_SPEED_KP_DEN;
    pwm += ((*integral) * MOTOR_SPEED_KI_NUM) / MOTOR_SPEED_KI_DEN;

    /*
     * 速度目标为正时，闭环只能把 PWM 降到 0，不能直接反向刹车。
     * 否则悬空轮一旦超速，PI 会在正反之间来回抽动，表现成某一侧突然高速。
     */
    if ((target > 0) && (pwm < 0)) {
        pwm = 0;
        if (*integral < 0) {
            *integral = 0;
        }
    } else if ((target < 0) && (pwm > 0)) {
        pwm = 0;
        if (*integral > 0) {
            *integral = 0;
        }
    }

    return Motor_ClampSpeed(pwm);
}

void Motor_Init(void)
{
    /* 先打开 TB6612 的 STBY，再主动停止两路 H 桥。 */
    PWM_Init();
    Motor_Enable();
    Motor_SpeedControlReset();
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

void Motor_SetTargetSpeed(int16_t leftTarget, int16_t rightTarget)
{
    leftTarget = Motor_ClampTargetSpeed(leftTarget);
    rightTarget = Motor_ClampTargetSpeed(rightTarget);

    if (((leftTarget > 0) && (gLeftTargetSpeed < 0)) ||
        ((leftTarget < 0) && (gLeftTargetSpeed > 0))) {
        gLeftIntegral = 0;
    }
    if (((rightTarget > 0) && (gRightTargetSpeed < 0)) ||
        ((rightTarget < 0) && (gRightTargetSpeed > 0))) {
        gRightIntegral = 0;
    }

    gLeftTargetSpeed = leftTarget;
    gRightTargetSpeed = rightTarget;
}

void Motor_SpeedControlUpdate(void)
{
    gLeftPwmOutput = Motor_CalcPwmBySpeedPI(
        gLeftTargetSpeed, Encoder_GetLeftSpeed(), &gLeftIntegral);
    gRightPwmOutput = Motor_CalcPwmBySpeedPI(
        gRightTargetSpeed, Encoder_GetRightSpeed(), &gRightIntegral);

    if ((gLeftPwmOutput == 0) && (gRightPwmOutput == 0)) {
        Motor_Stop();
        return;
    }

    Motor_SetSpeed(gLeftPwmOutput, gRightPwmOutput);
}

void Motor_SpeedControlReset(void)
{
    gLeftTargetSpeed = 0;
    gRightTargetSpeed = 0;
    gLeftIntegral = 0;
    gRightIntegral = 0;
    gLeftPwmOutput = 0;
    gRightPwmOutput = 0;
    Motor_Stop();
}

int16_t Motor_GetLeftTargetSpeed(void)
{
    return gLeftTargetSpeed;
}

int16_t Motor_GetRightTargetSpeed(void)
{
    return gRightTargetSpeed;
}

int16_t Motor_GetLeftPwmOutput(void)
{
    return gLeftPwmOutput;
}

int16_t Motor_GetRightPwmOutput(void)
{
    return gRightPwmOutput;
}
