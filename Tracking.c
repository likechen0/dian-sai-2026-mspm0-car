#include "Tracking.h"
#include "Motor.h"
#include "ti_msp_dl_config.h"

/*
 * 龙邱 8 路灰度循迹模块。
 * 模块只有一个模拟输出 AS，靠 S0/S1/S2 选择当前读哪一路。
 * 本驱动依次选择 8 路、读取 ADC，再计算加权循迹误差给导航状态机使用。
 */
unsigned char LQ_Tracking_Value[TRACKING_CHANNEL_COUNT] = {0};
volatile int16_t Tracking_Error = 0;
volatile int16_t Tracking_Correction = 0;
volatile uint8_t Tracking_LineDetected = 0;

static int16_t Tracking_LastError = 0;
static int16_t Tracking_LastControlError = 0;

static void Tracking_ShortDelay(void)
{
    /* 切换 S0/S1/S2 后，给模拟多路选择器一点稳定时间。 */
    delay_cycles(CPUCLK_FREQ / 200000U);
}

static int16_t Tracking_LimitSpeed(int32_t speed)
{
    if (speed > MOTOR_TARGET_SPEED_MAX) {
        return MOTOR_TARGET_SPEED_MAX;
    }
    if (speed < -MOTOR_TARGET_SPEED_MAX) {
        return -MOTOR_TARGET_SPEED_MAX;
    }
    return (int16_t)speed;
}

static uint16_t Tracking_GetLineStrength(uint8_t index)
{
    uint16_t value = LQ_Tracking_Value[index];

#if !TRACKING_BLACK_IS_HIGH
    /* 统一成“数值越大，越像黑线”。 */
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
    /* 先选择第 1 路，并使能 ADC 转换。 */
    Tracking_IO_Set(0, 0, 0);
    DL_ADC12_enableConversions(ADC12_0_INST);
}

void Tracking_IO_Set(unsigned char s2, unsigned char s1, unsigned char s0)
{
    uint32_t setPins = 0U;
    uint32_t clearPins = TRACKING_SEL_S0_PIN | TRACKING_SEL_S1_PIN | TRACKING_SEL_S2_PIN;

    /* 把通道选择位转换成 GPIO 置位/清零掩码。 */
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

    /* 启动一次单次 ADC 转换，并等待 MEM0 结果就绪。 */
    DL_ADC12_clearInterruptStatus(ADC12_0_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
    DL_ADC12_startConversion(ADC12_0_INST);

    while ((DL_ADC12_getRawInterruptStatus(
                ADC12_0_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED) == 0U) &&
           (timeout > 0U)) {
        timeout--;
    }

    if (timeout == 0U) {
        /* 超时保护：保持 ADC 可用，并返回一个近似“没线”的值。 */
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

    /*
     * 每一路切换后读多次。
     * 前几次丢弃，用来减小模拟多路选择器切换后的抖动。
     */
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

        /* 把 12 位 ADC 原始值缩放到 0..100，方便阈值判断。 */
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

    /* 刷新全局 8 路灰度数组。 */
    for (i = 0; i < TRACKING_CHANNEL_COUNT; i++) {
        LQ_Tracking_Value[i] = (unsigned char)Tracking_Value_once(i + 1U);
    }
}

int16_t Tracking_CalcError(void)
{
    /*
     * 左侧传感器权重为负，右侧传感器权重为正。
     * 加权平均后得到黑线相对中心的位置误差。
     */
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
        /* 没线时保留上次误差，这样重新看到线时不会突然跳变太大。 */
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
        /* 这个独立测试函数的策略：没线就停车。 */
        Tracking_Correction = 0;
        Motor_SpeedControlReset();
        return;
    }

    /* 这个函数保留给单独测试巡线用；当前主程序用 NAV 里的状态机。 */
    derivative = error - Tracking_LastControlError;
    Tracking_LastControlError = error;

    correction = ((int32_t)error * TRACKING_KP_NUM) / TRACKING_KP_DEN;
    correction += ((int32_t)derivative * TRACKING_KD_NUM) / TRACKING_KD_DEN;

    Tracking_Correction = Tracking_LimitSpeed(correction);

    leftSpeed = Tracking_LimitSpeed((int32_t)TRACKING_BASE_SPEED + Tracking_Correction);
    rightSpeed = Tracking_LimitSpeed((int32_t)TRACKING_BASE_SPEED - Tracking_Correction);

    Motor_SetTargetSpeed(leftSpeed, rightSpeed);
}
