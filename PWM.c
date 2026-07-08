#include "PWM.h"
#include "ti_msp_dl_config.h"

static uint32_t PWM_DutyToCompare(uint32_t duty)
{
    if (duty > PWM_DUTY_MAX) {
        duty = PWM_DUTY_MAX;
    }

    uint32_t period = DL_Timer_getLoadValue(PWM_0_INST);

    return period - ((duty * period) / PWM_DUTY_MAX);
}

static void PWM_SetChannel(uint16_t duty, DL_TIMER_CC_INDEX channel)
{
    DL_TimerA_setCaptureCompareValue(PWM_0_INST, PWM_DutyToCompare(duty), channel);
}

void PWM_Init(void)
{
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
