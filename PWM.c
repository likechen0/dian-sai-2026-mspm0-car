#include "PWM.h"
#include "ti_msp_dl_config.h"

/*
 * 新扩展板的两路 PWM 不在同一个定时器上：
 *   PA7  -> TIMG7_CCP1  -> PWMB -> 逻辑左轮 U16
 *   PB14 -> TIMG12_CCP1 -> PWMA -> 逻辑右轮 U6
 * 两个定时器使用相同的分频和周期，因此占空比换算保持一致。
 */
static uint32_t PWM_DutyToCompare(GPTIMER_Regs *timer, uint32_t duty)
{
    uint32_t period;

    if (duty > PWM_DUTY_MAX) {
        duty = PWM_DUTY_MAX;
    }

    period = DL_Timer_getLoadValue(timer);
    return period - ((duty * period) / PWM_DUTY_MAX);
}

static void PWM_SetChannel(
    GPTIMER_Regs *timer, uint16_t duty, DL_TIMER_CC_INDEX channel)
{
    DL_Timer_setCaptureCompareValue(
        timer, PWM_DutyToCompare(timer, duty), channel);
}

void PWM_Init(void)
{
    PWM_SetDuty_L(0);
    PWM_SetDuty_R(0);
    DL_Timer_startCounter(PWM_LEFT_INST);
    DL_Timer_startCounter(PWM_RIGHT_INST);
}

void PWM_SetDuty_L(uint16_t duty)
{
    PWM_SetChannel(PWM_LEFT_INST, duty, GPIO_PWM_LEFT_C1_IDX);
}

void PWM_SetDuty_R(uint16_t duty)
{
    PWM_SetChannel(PWM_RIGHT_INST, duty, GPIO_PWM_RIGHT_C1_IDX);
}
