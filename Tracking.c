#include "Tracking.h"
#include "Motor.h"
#include "ti_msp_dl_config.h"

/*
 * 感为传感器采集层。
 *
 * 官方例程的核心思想有两个：
 * 1. 每路都有黑/白校准值，把原始 ADC 归一化到 0..4096。
 * 2. 白底黑线时，用 4096 - normal 得到黑线强度，再做加权平均。
 *
 * 当前默认校准值来自感为 Car_OPEN 示例，只作为能启动测试的初值。
 * 换安装高度、换赛道、换供电后，应把下面两组值换成你实际测到的
 * 白底 ADC 和黑线 ADC。
 */
static const uint16_t Tracking_CalWhite[TRACKING_CHANNEL_COUNT] = {
    1789U, 1949U, 1676U, 1435U, 1712U, 2000U, 1520U, 2007U
};

static const uint16_t Tracking_CalBlack[TRACKING_CHANNEL_COUNT] = {
    126U, 135U, 130U, 137U, 133U, 132U, 140U, 136U
};

uint16_t Ganv_Tracking_Raw[TRACKING_CHANNEL_COUNT] = {0};
uint16_t Ganv_Tracking_Normal[TRACKING_CHANNEL_COUNT] = {0};
volatile uint8_t Ganv_Tracking_LineMask = 0U;
volatile uint8_t Ganv_Tracking_WhiteMask = 0xFFU;

unsigned char LQ_Tracking_Value[TRACKING_CHANNEL_COUNT] = {0};
volatile int16_t Tracking_Error = 0;
volatile int16_t Tracking_Correction = 0;
volatile uint8_t Tracking_LineDetected = 0;

static int16_t Tracking_LastError = 0;
static int16_t Tracking_LastControlError = 0;

static void Tracking_ShortDelay(void)
{
    /* 地址线切换后给模拟开关一点稳定时间。 */
    delay_cycles(CPUCLK_FREQ / 100000U);
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

static uint16_t Tracking_Normalize(uint8_t index, uint16_t raw)
{
    uint16_t white = Tracking_CalWhite[index];
    uint16_t black = Tracking_CalBlack[index];
    uint32_t normal;

    if (white <= black) {
        return (uint16_t)(((uint32_t)raw * TRACKING_NORMAL_MAX) / 4095U);
    }

    if (raw <= black) {
        return 0U;
    }

    normal = ((uint32_t)(raw - black) * TRACKING_NORMAL_MAX) /
        (uint32_t)(white - black);

    if (normal > TRACKING_NORMAL_MAX) {
        normal = TRACKING_NORMAL_MAX;
    }

    return (uint16_t)normal;
}

static void Tracking_UpdateWhiteMask(uint8_t index, uint16_t raw)
{
    uint16_t white = Tracking_CalWhite[index];
    uint16_t black = Tracking_CalBlack[index];
    uint16_t temp;
    uint16_t grayWhite;
    uint16_t grayBlack;
    uint8_t bit = (uint8_t)(1U << index);

    if (white < black) {
        temp = white;
        white = black;
        black = temp;
    }

    grayWhite = (uint16_t)(((uint32_t)white * 2U + black) / 3U);
    grayBlack = (uint16_t)(((uint32_t)white + (uint32_t)black * 2U) / 3U);

    if (raw > grayWhite) {
        Ganv_Tracking_WhiteMask |= bit;
    } else if (raw < grayBlack) {
        Ganv_Tracking_WhiteMask &= (uint8_t)(~bit);
    }
}

static uint16_t Tracking_GetLineStrengthByIndex(uint8_t index)
{
    uint16_t normal;
    uint16_t strength;

    if (index >= TRACKING_CHANNEL_COUNT) {
        return 0U;
    }

    normal = Ganv_Tracking_Normal[index];
    if (normal >= TRACKING_NORMAL_MAX) {
        return 0U;
    }

    strength = (uint16_t)(TRACKING_NORMAL_MAX - normal);
    if (strength < TRACKING_LINE_THRESHOLD) {
        return 0U;
    }

    return (uint16_t)(strength - TRACKING_LINE_THRESHOLD);
}

static uint8_t Tracking_ChannelToStoreIndex(uint8_t channel)
{
#if TRACKING_REVERSE_ORDER
    return (uint8_t)(TRACKING_CHANNEL_COUNT - channel);
#else
    return (uint8_t)(channel - 1U);
#endif
}

static uint8_t Tracking_AddressBit(uint8_t address, uint8_t bit)
{
    uint8_t value = (uint8_t)((address >> bit) & 1U);

#if TRACKING_ADDR_INVERT
    value ^= 1U;
#endif

    return value;
}

uint8_t Tracking_IsSensorOnLine(uint8_t channel)
{
    if ((channel < 1U) || (channel > TRACKING_CHANNEL_COUNT)) {
        return 0U;
    }

    return ((Ganv_Tracking_LineMask & (uint8_t)(1U << (channel - 1U))) != 0U) ?
        1U : 0U;
}

uint8_t Tracking_GetLineMask(void)
{
    return Ganv_Tracking_LineMask;
}

uint16_t Tracking_GetLineStrength(uint8_t channel)
{
    if ((channel < 1U) || (channel > TRACKING_CHANNEL_COUNT)) {
        return 0U;
    }

    return Tracking_GetLineStrengthByIndex((uint8_t)(channel - 1U));
}

void Tracking_Init(void)
{
    Tracking_Adc_Init();
}

void Tracking_Adc_Init(void)
{
    Tracking_IO_Set(0U, 0U, 0U);
    DL_ADC12_enableConversions(ADC12_0_INST);
}

void Tracking_IO_Set(unsigned char ad2, unsigned char ad1, unsigned char ad0)
{
    uint32_t setPins = 0U;
    uint32_t clearPins = TRACKING_SEL_AD0_PIN |
        TRACKING_SEL_AD1_PIN | TRACKING_SEL_AD2_PIN;

    if (ad0 != 0U) {
        setPins |= TRACKING_SEL_AD0_PIN;
    }
    if (ad1 != 0U) {
        setPins |= TRACKING_SEL_AD1_PIN;
    }
    if (ad2 != 0U) {
        setPins |= TRACKING_SEL_AD2_PIN;
    }

    clearPins &= ~setPins;
    DL_GPIO_clearPins(TRACKING_SEL_PORT, clearPins);
    DL_GPIO_setPins(TRACKING_SEL_PORT, setPins);

    Tracking_ShortDelay();
}

uint16_t Tracking_Adc_once(void)
{
    uint32_t timeout = 100000U;
    uint16_t value;

    DL_ADC12_clearInterruptStatus(ADC12_0_INST,
        DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
    DL_ADC12_startConversion(ADC12_0_INST);

    while ((DL_ADC12_getRawInterruptStatus(ADC12_0_INST,
                DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED) == 0U) &&
           (timeout > 0U)) {
        timeout--;
    }

    if (timeout == 0U) {
        DL_ADC12_enableConversions(ADC12_0_INST);
        return 0U;
    }

    value = DL_ADC12_getMemResult(ADC12_0_INST, ADC12_0_ADCMEM_0);
    DL_ADC12_enableConversions(ADC12_0_INST);

    return value;
}

unsigned int Tracking_Value_once(unsigned char ch)
{
    uint8_t i;
    uint8_t address;
    uint32_t sum = 0U;
    const uint8_t sampleCount = 5U;
    const uint8_t discardCount = 2U;

    if ((ch < 1U) || (ch > TRACKING_CHANNEL_COUNT)) {
        ch = 1U;
    }

    address = (uint8_t)(ch - 1U);
    Tracking_IO_Set(
        Tracking_AddressBit(address, 2U),
        Tracking_AddressBit(address, 1U),
        Tracking_AddressBit(address, 0U));

    for (i = 0U; i < sampleCount; i++) {
        uint16_t raw = Tracking_Adc_once();

        if (i >= discardCount) {
            sum += raw;
        }
    }

    return (unsigned int)(sum / (sampleCount - discardCount));
}

void Tracking_Value_Acquire(void)
{
    uint8_t ch;
    uint8_t index;
    uint16_t raw;
    uint16_t normal;
    uint16_t strength;
    uint8_t lineMask = 0U;

    for (ch = 1U; ch <= TRACKING_CHANNEL_COUNT; ch++) {
        index = Tracking_ChannelToStoreIndex(ch);
        raw = (uint16_t)Tracking_Value_once(ch);
        normal = Tracking_Normalize(index, raw);

        Ganv_Tracking_Raw[index] = raw;
        Ganv_Tracking_Normal[index] = normal;
        Tracking_UpdateWhiteMask(index, raw);

        strength = Tracking_GetLineStrengthByIndex(index);
        LQ_Tracking_Value[index] = (unsigned char)
            (((uint32_t)strength * TRACKING_VALUE_MAX) /
             (TRACKING_NORMAL_MAX - TRACKING_LINE_THRESHOLD));

        if (strength > 0U) {
            lineMask |= (uint8_t)(1U << index);
        }
    }

    Ganv_Tracking_LineMask = lineMask;
}

int16_t Tracking_CalcError(void)
{
    static const int16_t weight[TRACKING_CHANNEL_COUNT] = {
        -3500, -2500, -1500, -500, 500, 1500, 2500, 3500
    };
    uint8_t i;
    uint16_t strength;
    uint32_t total = 0U;
    int32_t weightedSum = 0;

    for (i = 0U; i < TRACKING_CHANNEL_COUNT; i++) {
        strength = Tracking_GetLineStrengthByIndex(i);
        total += strength;
        weightedSum += (int32_t)strength * weight[i];
    }

    if (total == 0U) {
        Tracking_LineDetected = 0U;
        Tracking_Error = Tracking_LastError;
        return Tracking_Error;
    }

    Tracking_LineDetected = 1U;
    Tracking_Error = (int16_t)(weightedSum / (int32_t)total);
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

    leftSpeed = Tracking_LimitSpeed((int32_t)TRACKING_BASE_SPEED +
        Tracking_Correction);
    rightSpeed = Tracking_LimitSpeed((int32_t)TRACKING_BASE_SPEED -
        Tracking_Correction);

    Motor_SetSpeed(leftSpeed, rightSpeed);
}
