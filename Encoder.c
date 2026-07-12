#include "Encoder.h"
#include "MS901M.h"
#include "ti_msp_dl_config.h"

/*
 * 增量式编码器模块。
 * 这里只用 A 相触发中断，在中断里读取 B 相来判断方向。
 * Encoder_Update() 每隔 ENCODER_SAMPLE_MS 取一次脉冲数，作为有符号速度值。
 */
volatile int16_t Encoder_LeftSpeed;
volatile int16_t Encoder_RightSpeed;

/* 中断里累加的脉冲数，Encoder_Update() 读取后会清零。 */
static volatile int32_t gEncoderLeftCount;
static volatile int32_t gEncoderRightCount;
static int32_t gEncoderLeftTotal;
static int32_t gEncoderRightTotal;
static int32_t gOdomXmm;
static int32_t gOdomYmm;
static int16_t gOdomYawCdeg;

static const int16_t gSin0To90Deg1000[] = {
    0, 87, 174, 259, 342, 423, 500, 574, 643, 707,
    766, 819, 866, 906, 940, 966, 985, 996, 1000
};

static int32_t Encoder_ClampToInt16(int32_t value)
{
    if (value > 32767) {
        return 32767;
    }
    if (value < -32768) {
        return -32768;
    }
    return value;
}

static int16_t Encoder_Sin0To90Cdeg1000(int32_t angleCdeg)
{
    int32_t index = angleCdeg / 500;
    int32_t remain = angleCdeg % 500;
    int32_t base;
    int32_t next;

    if (index >= 18) {
        return 1000;
    }

    base = gSin0To90Deg1000[index];
    next = gSin0To90Deg1000[index + 1];
    return (int16_t)(base + ((next - base) * remain) / 500);
}

static int16_t Encoder_SinCdeg1000(int32_t angleCdeg)
{
    while (angleCdeg < 0) {
        angleCdeg += 36000;
    }
    while (angleCdeg >= 36000) {
        angleCdeg -= 36000;
    }

    if (angleCdeg <= 9000) {
        return Encoder_Sin0To90Cdeg1000(angleCdeg);
    }
    if (angleCdeg <= 18000) {
        return Encoder_Sin0To90Cdeg1000(18000 - angleCdeg);
    }
    if (angleCdeg <= 27000) {
        return (int16_t)-Encoder_Sin0To90Cdeg1000(angleCdeg - 18000);
    }
    return (int16_t)-Encoder_Sin0To90Cdeg1000(36000 - angleCdeg);
}

static int16_t Encoder_CosCdeg1000(int32_t angleCdeg)
{
    return Encoder_SinCdeg1000(9000 - angleCdeg);
}

static void Encoder_UpdateOdometry(int16_t leftCount, int16_t rightCount)
{
    int32_t averageCount = ((int32_t)leftCount + rightCount) / 2;
    int32_t distanceMm =
        (averageCount * ENCODER_MM_PER_COUNT_NUM) / ENCODER_MM_PER_COUNT_DEN;

    gEncoderLeftTotal += leftCount;
    gEncoderRightTotal += rightCount;

    if (MS901M_Available()) {
        gOdomYawCdeg = MS901M_GetYawCdeg();
    }

    gOdomXmm += (distanceMm * Encoder_CosCdeg1000(gOdomYawCdeg)) / 1000;
    gOdomYmm += (distanceMm * Encoder_SinCdeg1000(gOdomYawCdeg)) / 1000;
}

static void Encoder_HandleLeftEdge(void)
{
    uint32_t a = DL_GPIO_readPins(ENCODER_PORTB_PORT, ENCODER_PORTB_LEFT_A_PIN);
    uint32_t b = DL_GPIO_readPins(ENCODER_PORTB_PORT, ENCODER_PORTB_LEFT_B_PIN);

    /* 正交编码方向判断：A/B 相同是一种方向，不同是另一种方向。 */
    if ((a != 0U) == (b != 0U)) {
        gEncoderLeftCount += ENCODER_LEFT_DIR;
    } else {
        gEncoderLeftCount -= ENCODER_LEFT_DIR;
    }
}

static void Encoder_HandleRightEdge(void)
{
    uint32_t a = DL_GPIO_readPins(ENCODER_PORTB_PORT, ENCODER_PORTB_RIGHT_A_PIN);
    uint32_t b = DL_GPIO_readPins(ENCODER_PORTB_PORT, ENCODER_PORTB_RIGHT_B_PIN);

    /* ENCODER_RIGHT_DIR 用来在软件里修正右轮方向。 */
    if ((a != 0U) == (b != 0U)) {
        gEncoderRightCount += ENCODER_RIGHT_DIR;
    } else {
        gEncoderRightCount -= ENCODER_RIGHT_DIR;
    }
}

void Encoder_Init(void)
{
    gEncoderLeftCount = 0;
    gEncoderRightCount = 0;
    Encoder_LeftSpeed = 0;
    Encoder_RightSpeed = 0;
    Encoder_ResetOdometry();

    /* 使能两个编码器 A 相所在 GPIO 组中断。 */
    DL_GPIO_clearInterruptStatus(ENCODER_PORTB_PORT,
        ENCODER_PORTB_LEFT_A_PIN | ENCODER_PORTB_RIGHT_A_PIN);
    NVIC_EnableIRQ(ENCODER_PORTB_INT_IRQN);
}

void Encoder_Update(void)
{
    int32_t left;
    int32_t right;

    /*
     * 关中断后复制并清零，避免中断刚好改到一半。
     * 这里得到的是每 ENCODER_SAMPLE_MS 的脉冲数，不是 RPM。
     */
    __disable_irq();
    left = gEncoderLeftCount;
    right = gEncoderRightCount;
    gEncoderLeftCount = 0;
    gEncoderRightCount = 0;
    __enable_irq();

    Encoder_LeftSpeed = (int16_t)Encoder_ClampToInt16(left);
    Encoder_RightSpeed = (int16_t)Encoder_ClampToInt16(right);
    Encoder_UpdateOdometry(Encoder_LeftSpeed, Encoder_RightSpeed);
}

int16_t Encoder_GetLeftSpeed(void)
{
    return Encoder_LeftSpeed;
}

int16_t Encoder_GetRightSpeed(void)
{
    return Encoder_RightSpeed;
}

int32_t Encoder_GetLeftTotalCount(void)
{
    return gEncoderLeftTotal;
}

int32_t Encoder_GetRightTotalCount(void)
{
    return gEncoderRightTotal;
}

int32_t Encoder_GetXmm(void)
{
    return gOdomXmm;
}

int32_t Encoder_GetYmm(void)
{
    return gOdomYmm;
}

int16_t Encoder_GetPoseYawCdeg(void)
{
    return gOdomYawCdeg;
}

void Encoder_ResetOdometry(void)
{
    gEncoderLeftTotal = 0;
    gEncoderRightTotal = 0;
    gOdomXmm = 0;
    gOdomYmm = 0;
    gOdomYawCdeg = 0;
}

int32_t Encoder_CountToRPM(int16_t count, uint16_t sampleMs, uint16_t countsPerRev)
{
    if ((sampleMs == 0U) || (countsPerRev == 0U)) {
        return 0;
    }

    /* count/sampleMs -> 每分钟脉冲数 -> 每分钟转数。 */
    return ((int32_t)count * 60000) / ((int32_t)sampleMs * countsPerRev);
}

void GROUP1_IRQHandler(void)
{
    /* GROUP1 里可能有多个 GPIO 中断，这里只处理编码器端口事件。 */
    switch (DL_Interrupt_getPendingGroup(DL_INTERRUPT_GROUP_1)) {
        case ENCODER_PORTB_INT_IIDX: {
            uint32_t status = DL_GPIO_getEnabledInterruptStatus(ENCODER_PORTB_PORT,
                ENCODER_PORTB_LEFT_A_PIN | ENCODER_PORTB_RIGHT_A_PIN);

            DL_GPIO_clearInterruptStatus(ENCODER_PORTB_PORT, status);

            /* 哪个轮子的 A 相产生边沿，就更新哪个轮子的计数。 */
            if ((status & ENCODER_PORTB_LEFT_A_PIN) != 0U) {
                Encoder_HandleLeftEdge();
            }
            if ((status & ENCODER_PORTB_RIGHT_A_PIN) != 0U) {
                Encoder_HandleRightEdge();
            }
            break;
        }
        default:
            break;
    }
}
