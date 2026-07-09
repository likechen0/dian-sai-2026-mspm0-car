#include "ti_msp_dl_config.h"
#include "Motor.h"
#include "Encoder.h"
#include "Delay.h"
#include "Tracking.h"
#include "Display.h"
#include "InertialNav.h"

/*
 * 主程序入口。
 * 引脚和外设初始化由 SysConfig 生成的 SYSCFG_DL_init() 完成。
 * while 循环大约每 10ms 跑一次导航控制。
 */
int main(void)
{
    SYSCFG_DL_init();

    /* 先把各个模块初始化到安全状态，再允许电机动作。 */
    Motor_Init();
    Encoder_Init();
    Tracking_Init();
    NAV_Init();
    Display_Init();

    while (1) {
        /* 读取传感器并决定：巡线、白地直行，还是陀螺仪转向。 */
        NAV_ControlStep();

        /* 编码器速度按这个固定时间窗采样。 */
        Delay_ms(ENCODER_SAMPLE_MS);
        Encoder_Update();

        /* 速度内环：根据目标脉冲速度和编码器反馈，计算真正输出到 TB6612 的 PWM。 */
        Motor_SpeedControlUpdate();

        /* OLED 内部做了分频刷新，所以主循环里每次调用也没问题。 */
        Display_Update();

        /* 运行心跳灯：闪烁说明主循环还活着。 */
        DL_GPIO_togglePins(RUN_LED_PORT, RUN_LED_LED_PIN);
    }
}
