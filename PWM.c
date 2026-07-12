#include "PWM.h"
#include "ti_msp_dl_config.h"

/*
 * TB6612 PWM module.
 *
 * heyvhao uses TIMG0:
 * channel 0 -> PA12 -> PWMA -> left wheel
 * channel 1 -> PA13 -> PWMB -> right wheel
 */
static uint32_t PWM_DutyToCompare(uint32_t duty)
{
    uint32_t period;

    if (duty > PWM_DUTY_MAX) {
        duty = PWM_DUTY_MAX;
    }

    period = DL_Timer_getLoadValue(PWM_0_INST);

    return period - ((duty * period) / PWM_DUTY_MAX);
}

static void PWM_SetChannel(uint16_t duty, DL_TIMER_CC_INDEX channel)
{
    DL_Timer_setCaptureCompareValue(PWM_0_INST,
        PWM_DutyToCompare(duty), channel);
}

void PWM_Init(void)
{
    PWM_SetDuty_L(0);
    PWM_SetDuty_R(0);
    DL_Timer_startCounter(PWM_0_INST);
}

void PWM_SetDuty_L(uint16_t duty)
{
    PWM_SetChannel(duty, GPIO_PWM_0_C0_IDX);
}

void PWM_SetDuty_R(uint16_t duty)
{
    PWM_SetChannel(duty, GPIO_PWM_0_C1_IDX);
}
