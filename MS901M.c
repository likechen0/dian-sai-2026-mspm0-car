#include "MS901M.h"
#include "ti_msp_dl_config.h"

/*
 * MS901M parser for the common 0x55 frame stream.
 * Supported angle frame:
 *   55 53 RollL RollH PitchL PitchH YawL YawH VL VH SUM
 * Angles are stored in centidegrees: 4500 means 45.00 deg.
 */
static volatile int16_t gYawRawCdeg;
static volatile int16_t gRollCdeg;
static volatile int16_t gPitchCdeg;
static volatile int16_t gYawZeroCdeg;
static volatile uint16_t gRxByteCount;
static volatile uint16_t gAngleFrameCount;
static volatile uint16_t gBadFrameCount;
static volatile uint8_t gLastFrameId;
static volatile uint8_t gLastErrorCode;
static volatile bool gAngleOk;

static int16_t MS901M_ToInt16(const uint8_t *p)
{
    return (int16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static int16_t MS901M_Wrap180(int32_t angle)
{
    while (angle > 18000) {
        angle -= 36000;
    }
    while (angle < -18000) {
        angle += 36000;
    }

    return (int16_t)angle;
}

static uint16_t MS901M_CountUp(uint16_t value)
{
    return (value < 9999U) ? (uint16_t)(value + 1U) : value;
}

static void MS901M_ParseFrame(const uint8_t *buffer)
{
    uint8_t sum = 0;

    for (uint8_t i = 0; i < 10U; i++) {
        sum += buffer[i];
    }

    gLastFrameId = buffer[1];

    if (sum != buffer[10]) {
        gBadFrameCount = MS901M_CountUp(gBadFrameCount);
        return;
    }

    if (buffer[1] == 0x53U) {
        int16_t rollRaw = MS901M_ToInt16(&buffer[2]);
        int16_t pitchRaw = MS901M_ToInt16(&buffer[4]);
        int16_t yawRaw = MS901M_ToInt16(&buffer[6]);

        gRollCdeg = (int16_t)(((int32_t)rollRaw * 18000) / 32768);
        gPitchCdeg = (int16_t)(((int32_t)pitchRaw * 18000) / 32768);
        gYawRawCdeg = (int16_t)(((int32_t)yawRaw * 18000) / 32768);
        gAngleFrameCount = MS901M_CountUp(gAngleFrameCount);
        gAngleOk = true;
    }
}

void MS901M_Init(void)
{
    gYawRawCdeg = 0;
    gRollCdeg = 0;
    gPitchCdeg = 0;
    gYawZeroCdeg = 0;
    gRxByteCount = 0;
    gAngleFrameCount = 0;
    gBadFrameCount = 0;
    gLastFrameId = 0;
    gLastErrorCode = 0;
    gAngleOk = false;

    DL_UART_Main_clearInterruptStatus(UART_0_INST,
        DL_UART_MAIN_INTERRUPT_RX |
        DL_UART_MAIN_INTERRUPT_RX_TIMEOUT_ERROR |
        DL_UART_MAIN_INTERRUPT_OVERRUN_ERROR |
        DL_UART_MAIN_INTERRUPT_FRAMING_ERROR |
        DL_UART_MAIN_INTERRUPT_NOISE_ERROR |
        DL_UART_MAIN_INTERRUPT_PARITY_ERROR |
        DL_UART_MAIN_INTERRUPT_BREAK_ERROR);
    NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_0_INST_INT_IRQN);
}

void MS901M_PushByte(uint8_t byte)
{
    static uint8_t buffer[11];
    static uint8_t index = 0;

    gRxByteCount = MS901M_CountUp(gRxByteCount);

    if (index == 0U) {
        if (byte == 0x55U) {
            buffer[index++] = byte;
        }
        return;
    }

    buffer[index++] = byte;

    if (index == 2U) {
        if ((buffer[1] < 0x51U) || (buffer[1] > 0x53U)) {
            index = (byte == 0x55U) ? 1U : 0U;
            buffer[0] = 0x55U;
        }
        return;
    }

    if (index >= 11U) {
        MS901M_ParseFrame(buffer);
        index = 0;
    }
}

bool MS901M_Available(void)
{
    return gAngleOk;
}

int16_t MS901M_GetYawCdeg(void)
{
    return MS901M_Wrap180((int32_t)gYawRawCdeg - gYawZeroCdeg);
}

int16_t MS901M_GetRollCdeg(void)
{
    return gRollCdeg;
}

int16_t MS901M_GetPitchCdeg(void)
{
    return gPitchCdeg;
}

uint16_t MS901M_GetRxByteCount(void)
{
    return gRxByteCount;
}

uint16_t MS901M_GetAngleFrameCount(void)
{
    return gAngleFrameCount;
}

uint16_t MS901M_GetBadFrameCount(void)
{
    return gBadFrameCount;
}

uint8_t MS901M_GetLastFrameId(void)
{
    return gLastFrameId;
}

uint8_t MS901M_GetLastErrorCode(void)
{
    return gLastErrorCode;
}

void MS901M_SetYawZero(void)
{
    gYawZeroCdeg = gYawRawCdeg;
}

int16_t MS901M_YawErrorCdeg(int16_t target, int16_t current)
{
    return MS901M_Wrap180((int32_t)target - current);
}

static void MS901M_DrainRxFifo(void)
{
    while (!DL_UART_Main_isRXFIFOEmpty(UART_0_INST)) {
        MS901M_PushByte(DL_UART_Main_receiveData(UART_0_INST));
    }
}

void UART_0_INST_IRQHandler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(UART_0_INST)) {
        case DL_UART_MAIN_IIDX_RX:
        case DL_UART_MAIN_IIDX_RX_TIMEOUT_ERROR:
            MS901M_DrainRxFifo();
            break;

        case DL_UART_MAIN_IIDX_OVERRUN_ERROR:
            gLastErrorCode = 1U;
            gBadFrameCount = MS901M_CountUp(gBadFrameCount);
            MS901M_DrainRxFifo();
            break;

        case DL_UART_MAIN_IIDX_FRAMING_ERROR:
            gLastErrorCode = 2U;
            gBadFrameCount = MS901M_CountUp(gBadFrameCount);
            MS901M_DrainRxFifo();
            break;

        case DL_UART_MAIN_IIDX_NOISE_ERROR:
            gLastErrorCode = 3U;
            gBadFrameCount = MS901M_CountUp(gBadFrameCount);
            MS901M_DrainRxFifo();
            break;

        case DL_UART_MAIN_IIDX_PARITY_ERROR:
            gLastErrorCode = 4U;
            gBadFrameCount = MS901M_CountUp(gBadFrameCount);
            MS901M_DrainRxFifo();
            break;

        case DL_UART_MAIN_IIDX_BREAK_ERROR:
            gLastErrorCode = 5U;
            gBadFrameCount = MS901M_CountUp(gBadFrameCount);
            MS901M_DrainRxFifo();
            break;

        default:
            break;
    }
}
