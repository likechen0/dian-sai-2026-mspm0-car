#include "ti_msp_dl_config.h"
#include "Motor.h"
#include "Encoder.h"
#include "Tracking.h"
#include "Display.h"
#include "InertialNav.h"
#include "MS901M.h"

/*
 * 循迹运行模式（编译期切换，改完重新编译烧录）。
 *
 *   0 = 调试模式：小车静止，OLED 显示传感器数据。
 *   1 = 巡线模式：10 ms 定时中断运行 NAV_ControlStep()。
 *
 * 显示内容由 Display.h 中的 TRACKING_DISPLAY_MODE 选择。
 * PID 调参入口在 Tracking.h。
 */
#define TRACKING_RUN_MODE 1U

#define CONTROL_PERIOD_MS 10U
#define RUN_LED_PERIOD_MS 500U

#if (ENCODER_SAMPLE_MS != CONTROL_PERIOD_MS)
#error "ENCODER_SAMPLE_MS must match the SysConfig control timer period"
#endif

#if ((DISPLAY_UPDATE_PERIOD_MS % CONTROL_PERIOD_MS) != 0U)
#error "DISPLAY_UPDATE_PERIOD_MS must be a multiple of CONTROL_PERIOD_MS"
#endif

#if ((RUN_LED_PERIOD_MS % CONTROL_PERIOD_MS) != 0U)
#error "RUN_LED_PERIOD_MS must be a multiple of CONTROL_PERIOD_MS"
#endif

#define DISPLAY_UPDATE_TICKS (DISPLAY_UPDATE_PERIOD_MS / CONTROL_PERIOD_MS)
#define RUN_LED_TICKS (RUN_LED_PERIOD_MS / CONTROL_PERIOD_MS)

static volatile uint8_t gDisplayUpdateDue;

static void App_ControlStep(void)
{
    /* 在严格的 10 ms 边界先锁存编码器计数。 */
    Encoder_Update();

#if TRACKING_RUN_MODE
    NAV_ControlStep();
#else
    Tracking_Value_Acquire();
    Tracking_CalcError();
    Motor_Stop();
#endif
}

static uint8_t App_TakeDisplayRequest(void)
{
    uint8_t due;

    __disable_irq();
    due = gDisplayUpdateDue;
    gDisplayUpdateDue = 0U;
    __enable_irq();

    return due;
}

int main(void)
{
    SYSCFG_DL_init();

    Motor_Init();
    Encoder_Init();
    Tracking_Init();
    Display_Init();
    /* OLED 清屏较慢，完成后再开启 MS901M UART 接收。 */
    NAV_Init();

    gDisplayUpdateDue = 1U;

    /*
     * 编码器和 UART 接收使用优先级 0，可抢占优先级 1 的控制环。
     * 控制定时器在所有模块初始化完成后才启动。
     */
    NVIC_SetPriority(CONTROL_TIMER_INST_INT_IRQN, 1U);
    DL_TimerG_clearInterruptStatus(
        CONTROL_TIMER_INST, DL_TIMERG_INTERRUPT_ZERO_EVENT);
    NVIC_ClearPendingIRQ(CONTROL_TIMER_INST_INT_IRQN);
    NVIC_EnableIRQ(CONTROL_TIMER_INST_INT_IRQN);
    DL_TimerG_startCounter(CONTROL_TIMER_INST);

    while (1) {
        /* 协议解析和 OLED 都是非实时任务，允许控制中断随时抢占。 */
        MS901M_Process();

        if (App_TakeDisplayRequest() != 0U) {
            Display_Update();
        }

        __WFI();
    }
}

void CONTROL_TIMER_INST_IRQHandler(void)
{
    static uint16_t displayDivider;
    static uint16_t ledDivider;

    switch (DL_TimerG_getPendingInterrupt(CONTROL_TIMER_INST)) {
        case DL_TIMER_IIDX_ZERO:
            App_ControlStep();

            displayDivider++;
            if (displayDivider >= DISPLAY_UPDATE_TICKS) {
                displayDivider = 0U;
                gDisplayUpdateDue = 1U;
            }

            ledDivider++;
            if (ledDivider >= RUN_LED_TICKS) {
                ledDivider = 0U;
                DL_GPIO_togglePins(RUN_LED_PORT, RUN_LED_LED_PIN);
            }
            break;

        default:
            break;
    }
}
