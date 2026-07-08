#include "PWM.h"
#include "ti_msp_dl_config.h"

/*
 * TB6612 调速 PWM 模块。
 * SysConfig 把 PWM_0 通道 0 映射到左电机 PWM，把通道 1 映射到右电机 PWM。
 * 占空比用 0..10000 表示，方便 PID 和速度命令计算。
 */
static uint32_t PWM_DutyToCompare(uint32_t duty)
{
    if (duty > PWM_DUTY_MAX) {
        duty = PWM_DUTY_MAX;
    }

    uint32_t period = DL_Timer_getLoadValue(PWM_0_INST);

    /* 当前 PWM 输出是低有效，所以占空比要反算成定时器比较值。 */
    return period - ((duty * period) / PWM_DUTY_MAX);
}

static void PWM_SetChannel(uint16_t duty, DL_TIMER_CC_INDEX channel)
{
    DL_TimerA_setCaptureCompareValue(PWM_0_INST, PWM_DutyToCompare(duty), channel);
}

void PWM_Init(void)
{
    /* 启动定时器前先把两路占空比清零，避免上电乱动。 */
    PWM_SetDuty_L(0);
    PWM_SetDuty_R(0);
    DL_TimerA_startCounter(PWM_0_INST);
}

void PWM_SetDuty_L(uint16_t duty)
{
    PWM_SetChannel(duty, GPIO_PWM_0_C0_IDX);
}

void PWM_SetDuty_R(uint16_t duty)
{
    PWM_SetChannel(duty, GPIO_PWM_0_C1_IDX);
}
