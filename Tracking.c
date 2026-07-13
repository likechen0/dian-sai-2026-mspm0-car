#include "Tracking.h"
#include "Motor.h"
#include "ti_msp_dl_config.h"

/*
 * 每一路都要用实车安装高度、赛道和供电条件重新采集黑白值。
 * 当前默认值继承自 heyvhao，只用于首次上电检查，不代表新板已标定。
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

    /* 两个阈值构成迟滞，避免黑白交界处的 0/1 抖动。 */
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

/*
 * 两个 ADC 各启动一次序列转换：
 * ADC1 的 MEM0..2 对应 S1..S3；
 * ADC0 的 MEM0..4 对应 S4..S8。
 */
static uint8_t Tracking_ReadAllRaw(uint16_t raw[TRACKING_CHANNEL_COUNT])
{
    uint32_t timeout0 = 200000U;
    uint32_t timeout1 = 200000U;

    DL_ADC12_clearInterruptStatus(TRACKING_ADC0_INST,
        DL_ADC12_INTERRUPT_MEM4_RESULT_LOADED);
    DL_ADC12_clearInterruptStatus(TRACKING_ADC1_INST,
        DL_ADC12_INTERRUPT_MEM2_RESULT_LOADED);

    DL_ADC12_startConversion(TRACKING_ADC1_INST);
    DL_ADC12_startConversion(TRACKING_ADC0_INST);

    while ((DL_ADC12_getRawInterruptStatus(TRACKING_ADC0_INST,
                DL_ADC12_INTERRUPT_MEM4_RESULT_LOADED) == 0U) &&
           (timeout0 > 0U)) {
        timeout0--;
    }
    while ((DL_ADC12_getRawInterruptStatus(TRACKING_ADC1_INST,
                DL_ADC12_INTERRUPT_MEM2_RESULT_LOADED) == 0U) &&
           (timeout1 > 0U)) {
        timeout1--;
    }

    if ((timeout0 == 0U) || (timeout1 == 0U)) {
        DL_ADC12_enableConversions(TRACKING_ADC0_INST);
        DL_ADC12_enableConversions(TRACKING_ADC1_INST);
        return 0U;
    }

    raw[0] = DL_ADC12_getMemResult(TRACKING_ADC1_INST, DL_ADC12_MEM_IDX_0);
    raw[1] = DL_ADC12_getMemResult(TRACKING_ADC1_INST, DL_ADC12_MEM_IDX_1);
    raw[2] = DL_ADC12_getMemResult(TRACKING_ADC1_INST, DL_ADC12_MEM_IDX_2);
    raw[3] = DL_ADC12_getMemResult(TRACKING_ADC0_INST, DL_ADC12_MEM_IDX_0);
    raw[4] = DL_ADC12_getMemResult(TRACKING_ADC0_INST, DL_ADC12_MEM_IDX_1);
    raw[5] = DL_ADC12_getMemResult(TRACKING_ADC0_INST, DL_ADC12_MEM_IDX_2);
    raw[6] = DL_ADC12_getMemResult(TRACKING_ADC0_INST, DL_ADC12_MEM_IDX_3);
    raw[7] = DL_ADC12_getMemResult(TRACKING_ADC0_INST, DL_ADC12_MEM_IDX_4);

    DL_ADC12_enableConversions(TRACKING_ADC0_INST);
    DL_ADC12_enableConversions(TRACKING_ADC1_INST);
    return 1U;
}

uint8_t Tracking_IsSensorOnLine(uint8_t channel)
{
    if ((channel < 1U) || (channel > TRACKING_CHANNEL_COUNT)) {
        return 0U;
    }
    return ((Ganv_Tracking_LineMask & (uint8_t)(1U << (channel - 1U))) != 0U)
        ? 1U : 0U;
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
    DL_ADC12_enableConversions(TRACKING_ADC0_INST);
    DL_ADC12_enableConversions(TRACKING_ADC1_INST);
}

unsigned int Tracking_Value_once(unsigned char channel)
{
    uint16_t raw[TRACKING_CHANNEL_COUNT];

    if ((channel < 1U) || (channel > TRACKING_CHANNEL_COUNT)) {
        channel = 1U;
    }
    if (Tracking_ReadAllRaw(raw) == 0U) {
        return 0U;
    }
    return raw[channel - 1U];
}

void Tracking_Value_Acquire(void)
{
    uint16_t rawValue[TRACKING_CHANNEL_COUNT];
    uint8_t channel;
    uint8_t index;
    uint16_t normal;
    uint16_t strength;
    uint8_t lineMask = 0U;

    if (Tracking_ReadAllRaw(rawValue) == 0U) {
        for (index = 0U; index < TRACKING_CHANNEL_COUNT; index++) {
            Ganv_Tracking_Raw[index] = Tracking_CalWhite[index];
            Ganv_Tracking_Normal[index] = TRACKING_NORMAL_MAX;
            LQ_Tracking_Value[index] = 0U;
        }
        Ganv_Tracking_LineMask = 0U;
        Ganv_Tracking_WhiteMask = 0xFFU;
        return;
    }

    for (channel = 1U; channel <= TRACKING_CHANNEL_COUNT; channel++) {
        index = Tracking_ChannelToStoreIndex(channel);
        Ganv_Tracking_Raw[index] = rawValue[channel - 1U];
        normal = Tracking_Normalize(index, rawValue[channel - 1U]);
        Ganv_Tracking_Normal[index] = normal;
        Tracking_UpdateWhiteMask(index, rawValue[channel - 1U]);

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
    uint8_t index;
    uint16_t strength;
    uint32_t total = 0U;
    int32_t weightedSum = 0;

    for (index = 0U; index < TRACKING_CHANNEL_COUNT; index++) {
        strength = Tracking_GetLineStrengthByIndex(index);
        total += strength;
        weightedSum += (int32_t)strength * weight[index];
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

    leftSpeed = Tracking_LimitSpeed(
        (int32_t)TRACKING_BASE_SPEED + Tracking_Correction);
    rightSpeed = Tracking_LimitSpeed(
        (int32_t)TRACKING_BASE_SPEED - Tracking_Correction);
    Motor_SetSpeed(leftSpeed, rightSpeed);
}
