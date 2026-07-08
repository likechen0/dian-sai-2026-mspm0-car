#include "Encoder.h"
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
}

int16_t Encoder_GetLeftSpeed(void)
{
    return Encoder_LeftSpeed;
}

int16_t Encoder_GetRightSpeed(void)
{
    return Encoder_RightSpeed;
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
