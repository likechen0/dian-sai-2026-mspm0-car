#include "MS901M.h"
#include "ti_msp_dl_config.h"
#include <string.h>

/*
 * MS901M parser.
 *
 * Two common protocols are accepted:
 * - WitMotion style:
 *     55 53 RollL RollH PitchL PitchH YawL YawH VL VH SUM
 * - ATK-MS901M/ATK-IMU901 style:
 *     55 55 ID LEN DATA... SUM
 *   Euler angle frame: ID=01, LEN=06,
 *     DATA = RollL RollH PitchL PitchH YawL YawH
 *
 * Angles are stored in centidegrees: 4500 means 45.00 deg.
 */
#define MS901M_WIT_FRAME_SIZE         11U
#define MS901M_ATK_EULER_FRAME_ID     0x01U
#define MS901M_ATK_EULER_PAYLOAD_SIZE 6U
#define MS901M_ATK_MAX_PAYLOAD_SIZE   24U
#define MS901M_RX_BUFFER_SIZE         40U

static volatile int16_t gYawRawCdeg;
static volatile int16_t gRollCdeg;
static volatile int16_t gPitchCdeg;
static volatile int16_t gYawZeroCdeg;
static volatile uint16_t gRxByteCount;
static volatile uint16_t gAngleFrameCount;
static volatile uint16_t gBadFrameCount;
static volatile uint8_t gLastFrameId;
static volatile uint8_t gLastErrorCode;
static volatile uint8_t gRecentBytes[4];
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

static uint8_t MS901M_Checksum(const uint8_t *buffer, uint8_t length)
{
    uint8_t sum = 0;

    for (uint8_t i = 0; i < length; i++) {
        sum += buffer[i];
    }

    return sum;
}

static void MS901M_ParseEulerPayload(const uint8_t *payload)
{
    int16_t rollRaw = MS901M_ToInt16(&payload[0]);
    int16_t pitchRaw = MS901M_ToInt16(&payload[2]);
    int16_t yawRaw = MS901M_ToInt16(&payload[4]);

    gRollCdeg = (int16_t)(((int32_t)rollRaw * 18000) / 32768);
    gPitchCdeg = (int16_t)(((int32_t)pitchRaw * 18000) / 32768);
    gYawRawCdeg = (int16_t)(((int32_t)yawRaw * 18000) / 32768);
    gAngleFrameCount = MS901M_CountUp(gAngleFrameCount);
    gAngleOk = true;
}

static void MS901M_ParseWitFrame(const uint8_t *buffer)
{
    gLastFrameId = buffer[1];

    if (MS901M_Checksum(buffer, 10U) != buffer[10]) {
        gBadFrameCount = MS901M_CountUp(gBadFrameCount);
        return;
    }

    if (buffer[1] == 0x53U) {
        MS901M_ParseEulerPayload(&buffer[2]);
    }
}

static void MS901M_ParseAtkFrame(const uint8_t *buffer, uint8_t frameSize)
{
    uint8_t frameId = buffer[2];
    uint8_t payloadSize = buffer[3];

    gLastFrameId = frameId;

    if (MS901M_Checksum(buffer, (uint8_t)(frameSize - 1U)) != buffer[frameSize - 1U]) {
        gBadFrameCount = MS901M_CountUp(gBadFrameCount);
        return;
    }

    if ((frameId == MS901M_ATK_EULER_FRAME_ID) &&
        (payloadSize == MS901M_ATK_EULER_PAYLOAD_SIZE)) {
        MS901M_ParseEulerPayload(&buffer[4]);
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
    for (uint8_t i = 0; i < 4U; i++) {
        gRecentBytes[i] = 0;
    }
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
    static uint8_t buffer[MS901M_RX_BUFFER_SIZE];
    static uint8_t length = 0;

    gRxByteCount++;
    gRecentBytes[0] = gRecentBytes[1];
    gRecentBytes[1] = gRecentBytes[2];
    gRecentBytes[2] = gRecentBytes[3];
    gRecentBytes[3] = byte;

    if (length >= MS901M_RX_BUFFER_SIZE) {
        memmove(&buffer[0], &buffer[1], MS901M_RX_BUFFER_SIZE - 1U);
        length = MS901M_RX_BUFFER_SIZE - 1U;
        gBadFrameCount = MS901M_CountUp(gBadFrameCount);
    }

    buffer[length++] = byte;

    while (length > 0U) {
        if (buffer[0] != 0x55U) {
            memmove(&buffer[0], &buffer[1], length - 1U);
            length--;
            continue;
        }

        if (length < 2U) {
            return;
        }

        if (buffer[1] == 0x55U) {
            uint8_t payloadSize;
            uint8_t frameSize;

            if (length < 4U) {
                return;
            }

            payloadSize = buffer[3];
            if (payloadSize > MS901M_ATK_MAX_PAYLOAD_SIZE) {
                gLastFrameId = buffer[2];
                gBadFrameCount = MS901M_CountUp(gBadFrameCount);
                memmove(&buffer[0], &buffer[1], length - 1U);
                length--;
                continue;
            }

            frameSize = (uint8_t)(5U + payloadSize);
            if (length < frameSize) {
                return;
            }

            MS901M_ParseAtkFrame(buffer, frameSize);
            memmove(&buffer[0], &buffer[frameSize], length - frameSize);
            length = (uint8_t)(length - frameSize);
            continue;
        }

        if ((buffer[1] < 0x51U) || (buffer[1] > 0x53U)) {
            gLastFrameId = buffer[1];
            gBadFrameCount = MS901M_CountUp(gBadFrameCount);
            memmove(&buffer[0], &buffer[1], length - 1U);
            length--;
            continue;
        }

        if (length < MS901M_WIT_FRAME_SIZE) {
            return;
        }

        MS901M_ParseWitFrame(buffer);
        memmove(&buffer[0], &buffer[MS901M_WIT_FRAME_SIZE],
            length - MS901M_WIT_FRAME_SIZE);
        length = (uint8_t)(length - MS901M_WIT_FRAME_SIZE);
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

uint8_t MS901M_GetRecentByte(uint8_t index)
{
    if (index >= 4U) {
        return 0;
    }

    return gRecentBytes[index];
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
