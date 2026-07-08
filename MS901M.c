#include "MS901M.h"
#include "ti_msp_dl_config.h"

/*
 * MS901M / 维特风格串口解析器。
 * 模块输出 11 字节一帧：
 *   0x55、帧 ID、8 字节数据、校验和。
 * 当前代码只处理角度帧 0x53，并从中提取 roll / pitch / yaw 三个姿态角。
 */
static volatile int16_t gYawRawCdeg;
static volatile int16_t gRollCdeg;
static volatile int16_t gPitchCdeg;
static volatile int16_t gYawZeroCdeg;
static volatile bool gAngleOk;

static int16_t MS901M_ToInt16(const uint8_t *p)
{
    /* 传感器发送的是小端格式的有符号 16 位数据。 */
    return (int16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static int16_t MS901M_Wrap180(int32_t angle)
{
    /* 把 yaw 限制在 [-180.00, 180.00] 度，内部单位是 0.01 度。 */
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
    gRollCdeg = 0;
    gPitchCdeg = 0;
    gYawZeroCdeg = 0;
    gAngleOk = false;

    /* UART 引脚和波特率由 SysConfig 配置。 */
    NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_0_INST_INT_IRQN);
}

void MS901M_PushByte(uint8_t byte)
{
    static uint8_t buffer[11];
    static uint8_t index = 0;

    /* 用固定帧头 0x55 重新同步数据流。 */
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

    /* 0x53 是角度帧：2..3 是 roll，4..5 是 pitch，6..7 是 yaw。 */
    if ((sum == buffer[10]) && (buffer[1] == 0x53U)) {
        int16_t rollRaw = MS901M_ToInt16(&buffer[2]);
        int16_t pitchRaw = MS901M_ToInt16(&buffer[4]);
        int16_t yawRaw = MS901M_ToInt16(&buffer[6]);
        gRollCdeg = (int16_t)(((int32_t)rollRaw * 18000) / 32768);
        gPitchCdeg = (int16_t)(((int32_t)pitchRaw * 18000) / 32768);
        gYawRawCdeg = (int16_t)(((int32_t)yawRaw * 18000) / 32768);
        gAngleOk = true;
    }

    index = 0;
}

bool MS901M_Available(void)
{
    /* 至少收到一帧有效角度数据后返回 true。 */
    return gAngleOk;
}

int16_t MS901M_GetYawCdeg(void)
{
    /* 返回相对于当前零点的 yaw。 */
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

void MS901M_SetYawZero(void)
{
    /* 把当前原始 yaw 记为新的零点。 */
    gYawZeroCdeg = gYawRawCdeg;
}

int16_t MS901M_YawErrorCdeg(int16_t target, int16_t current)
{
    return MS901M_Wrap180((int32_t)target - current);
}

void UART_0_INST_IRQHandler(void)
{
    /* 快速读空 RX FIFO，并把字节喂给帧解析器。 */
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
