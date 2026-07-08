#include "Tracking.h"
#include "Motor.h"
#include "ti_msp_dl_config.h"

unsigned char LQ_Tracking_Value[TRACKING_CHANNEL_COUNT] = {0};
volatile int16_t Tracking_Error = 0;
volatile int16_t Tracking_Correction = 0;
volatile uint8_t Tracking_LineDetected = 0;

static int16_t Tracking_LastError = 0;
static int16_t Tracking_LastControlError = 0;

static void Tracking_ShortDelay(void)
{
    delay_cycles(CPUCLK_FREQ / 200000U);
}

static int16_t Tracking_LimitSpeed(int32_t speed)
{
    if (speed > MOTOR_SPEED_MAX) {
        return MOTOR_SPEED_MAX;
    }
    if (speed < -MOTOR_SPEED_MAX) {
        return -MOTOR_SPEED_MAX;
    }
    return (int16_t)speed;
}

static uint16_t Tracking_GetLineStrength(uint8_t index)
{
    uint16_t value = LQ_Tracking_Value[index];

#if !TRACKING_BLACK_IS_HIGH
    value = TRACKING_VALUE_MAX - value;
#endif

    if (value < TRACKING_LINE_THRESHOLD) {
        return 0;
    }

    return value - TRACKING_LINE_THRESHOLD;
}

void Tracking_Init(void)
{
    Tracking_Adc_Init();
}

void Tracking_Adc_Init(void)
{
    Tracking_IO_Set(0, 0, 0);
    DL_ADC12_enableConversions(ADC12_0_INST);
}

void Tracking_IO_Set(unsigned char s2, unsigned char s1, unsigned char s0)
{
    uint32_t setPins = 0U;
    uint32_t clearPins = TRACKING_SEL_S0_PIN | TRACKING_SEL_S1_PIN | TRACKING_SEL_S2_PIN;

    if (s0 != 0U) {
        setPins |= TRACKING_SEL_S0_PIN;
    }
    if (s1 != 0U) {
        setPins |= TRACKING_SEL_S1_PIN;
    }
    if (s2 != 0U) {
        setPins |= TRACKING_SEL_S2_PIN;
    }

    clearPins &= ~setPins;
    DL_GPIO_clearPins(TRACKING_SEL_PORT, clearPins);
    DL_GPIO_setPins(TRACKING_SEL_PORT, setPins);

    Tracking_ShortDelay();
}

uint16_t Tracking_Adc_once(void)
{
    uint32_t timeout = 100000U;

    DL_ADC12_clearInterruptStatus(ADC12_0_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
    DL_ADC12_startConversion(ADC12_0_INST);

    while ((DL_ADC12_getRawInterruptStatus(
                ADC12_0_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED) == 0U) &&
           (timeout > 0U)) {
        timeout--;
    }

    if (timeout == 0U) {
        DL_ADC12_enableConversions(ADC12_0_INST);
        return 0;
    }

    uint16_t value = DL_ADC12_getMemResult(ADC12_0_INST, ADC12_0_ADCMEM_0);
    DL_ADC12_enableConversions(ADC12_0_INST);
    return value;
}

unsigned int Tracking_Value_once(unsigned char ch)
{
    uint8_t i;
    uint16_t data;
    uint16_t sum = 0;
    const uint8_t numSamples = 5;
    const uint8_t discardSamples = 3;

    for (i = 0; i < numSamples; i++) {
        switch (ch) {
            case 1: Tracking_IO_Set(0, 0, 0); break;
            case 2: Tracking_IO_Set(0, 0, 1); break;
            case 3: Tracking_IO_Set(0, 1, 0); break;
            case 4: Tracking_IO_Set(0, 1, 1); break;
            case 5: Tracking_IO_Set(1, 0, 0); break;
            case 6: Tracking_IO_Set(1, 0, 1); break;
            case 7: Tracking_IO_Set(1, 1, 0); break;
            case 8: Tracking_IO_Set(1, 1, 1); break;
            default: Tracking_IO_Set(0, 0, 0); break;
        }

        data = (uint16_t)(((uint32_t)Tracking_Adc_once() * TRACKING_VALUE_MAX) / 4095U);
        if (data > TRACKING_VALUE_MAX) {
            data = TRACKING_VALUE_MAX;
        }

        if (i >= discardSamples) {
            sum += data;
        }
    }

    return sum / (numSamples - discardSamples);
}

void Tracking_Value_Acquire(void)
{
    uint8_t i;

    for (i = 0; i < TRACKING_CHANNEL_COUNT; i++) {
        LQ_Tracking_Value[i] = (unsigned char)Tracking_Value_once(i + 1U);
    }
}

int16_t Tracking_CalcError(void)
{
    static const int16_t weight[TRACKING_CHANNEL_COUNT] = {
        -3500, -2500, -1500, -500, 500, 1500, 2500, 3500
    };
    uint8_t i;
    uint16_t strength;
    uint16_t total = 0;
    int32_t weightedSum = 0;

    for (i = 0; i < TRACKING_CHANNEL_COUNT; i++) {
        strength = Tracking_GetLineStrength(i);
        total += strength;
        weightedSum += (int32_t)strength * weight[i];
    }

    if (total == 0U) {
        Tracking_LineDetected = 0;
        Tracking_Error = Tracking_LastError;
        return Tracking_Error;
    }

    Tracking_LineDetected = 1;
    Tracking_Error = (int16_t)(weightedSum / total);
    Tracking_LastError = Tracking_Error;

    return Tracking_Error;
}

void Tracking_LineFollowStep(void)
{
    int16_t error;
    int16_t derivative;
    int32_t correction;
    int16_t leftSpeed;
    int16_t rightSpeed;

    Tracking_Value_Acquire();
    error = Tracking_CalcError();

    if (Tracking_LineDetected == 0U) {
        Tracking_Correction = 0;
        Motor_Stop();
        return;
    }

    derivative = error - Tracking_LastControlError;
    Tracking_LastControlError = error;

    correction = ((int32_t)error * TRACKING_KP_NUM) / TRACKING_KP_DEN;
    correction += ((int32_t)derivative * TRACKING_KD_NUM) / TRACKING_KD_DEN;

    Tracking_Correction = Tracking_LimitSpeed(correction);

    leftSpeed = Tracking_LimitSpeed((int32_t)TRACKING_BASE_SPEED + Tracking_Correction);
    rightSpeed = Tracking_LimitSpeed((int32_t)TRACKING_BASE_SPEED - Tracking_Correction);

    Motor_SetSpeed(leftSpeed, rightSpeed);
}
