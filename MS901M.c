#include "MS901M.h"
#include "ti_msp_dl_config.h"

/*
 * MS901M/WitMotion-style UART parser.
 * The module outputs 11-byte frames:
 *   0x55, frame id, 8 payload bytes, checksum.
 * This code only consumes angle frames, id 0x53, and extracts yaw.
 */
static volatile int16_t gYawRawCdeg;
static volatile int16_t gYawZeroCdeg;
static volatile bool gAngleOk;

static int16_t MS901M_ToInt16(const uint8_t *p)
{
    /* The sensor sends little-endian signed 16-bit fields. */
    return (int16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static int16_t MS901M_Wrap180(int32_t angle)
{
    /* Keep yaw values in [-180.00, 180.00] degrees, stored as centidegrees. */
    while (angle > 18000) {
        angle -= 36000;
    }
    while (angle < -18000) {
        angle += 36000;
    }

    return (int16_t)angle;
}

void MS901M_Init(void)
{
    gYawRawCdeg = 0;
    gYawZeroCdeg = 0;
    gAngleOk = false;

    /* UART pins and baud rate are configured by SysConfig. */
    NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_0_INST_INT_IRQN);
}

void MS901M_PushByte(uint8_t byte)
{
    static uint8_t buffer[11];
    static uint8_t index = 0;

    /* Resynchronize on the fixed frame header. */
    if ((index == 0U) && (byte != 0x55U)) {
        return;
    }

    buffer[index++] = byte;

    if ((index == 2U) && ((buffer[1] < 0x51U) || (buffer[1] > 0x53U))) {
        index = 0;
        return;
    }

    if (index < 11U) {
        return;
    }

    uint8_t sum = 0;
    for (uint8_t i = 0; i < 10U; i++) {
        sum += buffer[i];
    }

    /* 0x53 is the angle frame; payload bytes 6..7 are yaw. */
    if ((sum == buffer[10]) && (buffer[1] == 0x53U)) {
        int16_t yawRaw = MS901M_ToInt16(&buffer[6]);
        gYawRawCdeg = (int16_t)(((int32_t)yawRaw * 18000) / 32768);
        gAngleOk = true;
    }

    index = 0;
}

bool MS901M_Available(void)
{
    /* True after at least one valid angle frame has arrived. */
    return gAngleOk;
}

int16_t MS901M_GetYawCdeg(void)
{
    /* Return yaw relative to the latest zero point. */
    return MS901M_Wrap180((int32_t)gYawRawCdeg - gYawZeroCdeg);
}

void MS901M_SetYawZero(void)
{
    /* Store the current raw yaw as the zero reference. */
    gYawZeroCdeg = gYawRawCdeg;
}

int16_t MS901M_YawErrorCdeg(int16_t target, int16_t current)
{
    return MS901M_Wrap180((int32_t)target - current);
}

void UART_0_INST_IRQHandler(void)
{
    /* Drain RX FIFO quickly and feed the byte stream parser. */
    switch (DL_UART_Main_getPendingInterrupt(UART_0_INST)) {
        case DL_UART_MAIN_IIDX_RX:
            while (!DL_UART_Main_isRXFIFOEmpty(UART_0_INST)) {
                MS901M_PushByte(DL_UART_Main_receiveData(UART_0_INST));
            }
            break;

        default:
            break;
    }
}
