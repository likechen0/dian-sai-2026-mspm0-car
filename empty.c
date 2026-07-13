#include "ti_msp_dl_config.h"
#include "Motor.h"
#include "Encoder.h"
#include "Delay.h"
#include "Tracking.h"
#include "Display.h"
#include "InertialNav.h"

/*
 * 循迹运行模式（编译期切换，改完重新编译烧录）。
 *
 *   0 = 调试模式：小车静止，OLED 显示传感器数据。
 *       显示内容由 Display.h 中的 TRACKING_DISPLAY_MODE 宏选择：
 *         0 → 01 检查（八路 0/1 状态）
 *         1 → ADC 获取（八路原始 ADC 值）
 *
 *   1 = 巡线模式：小车运行 NAV_ControlStep() 导航状态机，
 *       PD 循迹 / 白地直行 / 陀螺仪转向。
 *
 * 典型工作流：
 *   1. TRACKING_RUN_MODE=0, TRACKING_DISPLAY_MODE=1 → 采集黑白标定值
 *   2. 手动写入 Tracking.c 的 CalWhite/CalBlack
 *   3. TRACKING_RUN_MODE=0, TRACKING_DISPLAY_MODE=0 → 验证 01 区分正常
 *   4. TRACKING_RUN_MODE=1                     → 正式巡线，调 PD 参数
 *
 * 巡线调参入口: Tracking.h 中 TRACKING_KP_NUM/DEN, TRACKING_KD_NUM/DEN
 */
#define TRACKING_RUN_MODE 1U
int main(void)
{
    SYSCFG_DL_init();

    Motor_Init();
    Encoder_Init();
    Tracking_Init();
    NAV_Init();
    Display_Init();

    while (1) {
#if TRACKING_RUN_MODE
        /* 巡线模式：读传感器 → PD循迹/直行/陀螺仪转向 */
        NAV_ControlStep();
#else
        /*
         * 调试模式：只采集传感器数据刷新 OLED 显示，
         * 不运行导航状态机，小车保持静止。
         */
        Tracking_Value_Acquire();
        Tracking_CalcError();
        Motor_Stop();
#endif

        Delay_ms(ENCODER_SAMPLE_MS);
        Encoder_Update();
        Display_Update();

        DL_GPIO_togglePins(RUN_LED_PORT, RUN_LED_LED_PIN);
    }
}
