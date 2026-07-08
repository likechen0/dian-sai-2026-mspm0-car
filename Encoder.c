#include "Encoder.h"
#include "ti_msp_dl_config.h"

/*
 * Incremental encoder module.
 * Only channel A is used as the interrupt source; channel B is sampled in
 * the ISR to decide direction. Encoder_Update() snapshots the counts every
 * ENCODER_SAMPLE_MS and exposes them as signed speed-like counts.
 */
volatile int16_t Encoder_LeftSpeed;
volatile int16_t Encoder_RightSpeed;

/* ISR-side pulse accumulators. They are reset by Encoder_Update(). */
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

    /* Quadrature direction: A and B equal means one direction, unequal another. */
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

    /* ENCODER_RIGHT_DIR lets wiring direction be corrected in software. */
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

    /* Enable GPIO group interrupt for the two encoder A channels. */
    DL_GPIO_clearInterruptStatus(ENCODER_PORTB_PORT,
        ENCODER_PORTB_LEFT_A_PIN | ENCODER_PORTB_RIGHT_A_PIN);
    NVIC_EnableIRQ(ENCODER_PORTB_INT_IRQN);
}

void Encoder_Update(void)
{
    int32_t left;
    int32_t right;

    /*
     * Copy and clear atomically so the ISR cannot update half-read values.
     * The result is pulse count per ENCODER_SAMPLE_MS, not RPM.
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

    /* count/sampleMs -> count/minute -> revolutions/minute. */
    return ((int32_t)count * 60000) / ((int32_t)sampleMs * countsPerRev);
}

void GROUP1_IRQHandler(void)
{
    /* GROUP1 contains GPIO interrupts; filter for the encoder port event. */
    switch (DL_Interrupt_getPendingGroup(DL_INTERRUPT_GROUP_1)) {
        case ENCODER_PORTB_INT_IIDX: {
            uint32_t status = DL_GPIO_getEnabledInterruptStatus(ENCODER_PORTB_PORT,
                ENCODER_PORTB_LEFT_A_PIN | ENCODER_PORTB_RIGHT_A_PIN);

            DL_GPIO_clearInterruptStatus(ENCODER_PORTB_PORT, status);

            /* Handle whichever wheel produced an A-channel edge. */
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
