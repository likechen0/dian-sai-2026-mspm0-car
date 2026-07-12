#include "ti_msp_dl_config.h"
#include "Motor.h"
#include "Encoder.h"
#include "Delay.h"
#include "Tracking.h"
#include "Display.h"
#include "InertialNav.h"

/*
 * PD 巡线调参模式：
 * 小车运行 NAV_ControlStep() 导航状态机 (PD 循迹),
 * OLED 实时显示编码器速度、跟踪误差、PD 修正量、传感器状态。
 *
 * 调参入口: Tracking.h 中 TRACKING_KP_NUM/DEN, TRACKING_KD_NUM/DEN
 */
int main(void)
{
    SYSCFG_DL_init();

    Motor_Init();
    Encoder_Init();
    Tracking_Init();
    NAV_Init();
    Display_Init();

    while (1) {
        /* 导航状态机: 读传感器 → PD循迹/直行/陀螺仪转向 */
        NAV_ControlStep();

        Delay_ms(ENCODER_SAMPLE_MS);
        Encoder_Update();
        Display_Update();

        DL_GPIO_togglePins(RUN_LED_PORT, RUN_LED_LED_PIN);
    }
}
