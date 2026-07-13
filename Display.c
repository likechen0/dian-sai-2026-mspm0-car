#include "Display.h"
#include "OLED.h"
#include "Encoder.h"
#include "Tracking.h"
#include "InertialNav.h"

/*
 * 显示模式由 Display.h 中的 TRACKING_DISPLAY_MODE 宏控制：
 *   0 = 01 检查模式：显示八路 0/1 线状态 + 误差
 *   1 = ADC 获取模式：显示八路原始 ADC 值
 * 详见 Display.h 头注释。
 */

static void Display_ShowSensorMask(uint8_t line, uint8_t column, uint8_t mask)
{
    uint8_t i;

    for (i = 0U; i < TRACKING_CHANNEL_COUNT; i++) {
        OLED_ShowChar(line, (uint8_t)(column + i),
            ((mask & (uint8_t)(1U << i)) != 0U) ? '1' : '0');
    }
}

/*
 * 校准显示：一页显示 8 路原始 ADC。
 * 每行两路，4 行刚好 8 路。
 * 格式：1:1234 2:1234
 */
static void Display_ShowCalibration(void)
{
    uint8_t line;

    for (line = 0U; line < 4U; line++) {
        uint8_t chL = (uint8_t)(line * 2U + 1U);
        uint8_t chR = (uint8_t)(line * 2U + 2U);

        OLED_ShowNum((uint8_t)(line + 1U), 1, chL, 1);
        OLED_ShowChar((uint8_t)(line + 1U), 2, ':');
        OLED_ShowNum((uint8_t)(line + 1U), 3, Ganv_Tracking_Raw[chL - 1U], 4);

        OLED_ShowNum((uint8_t)(line + 1U), 9, chR, 1);
        OLED_ShowChar((uint8_t)(line + 1U), 10, ':');
        OLED_ShowNum((uint8_t)(line + 1U), 11, Ganv_Tracking_Raw[chR - 1U], 4);
    }
}

/* OLED 固定显示 4 位，防止异常脉冲数破坏行布局。 */
static int32_t Display_LimitEncoderCount(int32_t count)
{
    if (count > 9999) {
        return 9999;
    }
    if (count < -9999) {
        return -9999;
    }
    return count;
}

static void Display_ShowDebugInfo(
    int32_t leftWindowCount, int32_t rightWindowCount)
{
    /*
     * U16 的 A2/B2 作为逻辑左轮，U6 的 A1/B1 作为逻辑右轮。
     * 连续累加 10 个 10 ms 窗口，避免 OLED 恰好抽到无脉冲窗口。
     */
    OLED_ShowSignedNum(
        1, 3, Display_LimitEncoderCount(leftWindowCount), 4);
    OLED_ShowSignedNum(
        2, 3, Display_LimitEncoderCount(rightWindowCount), 4);

    OLED_ShowString(3, 1, "S:");
    Display_ShowSensorMask(3, 3, Tracking_GetLineMask());
    OLED_ShowString(3, 11, " C:");
    OLED_ShowNum(3, 14, NAV_GetLinePassCount(), 2);

    OLED_ShowString(4, 1, "E:");
    OLED_ShowSignedNum(4, 3, Tracking_Error, 4);
    OLED_ShowString(4, 8, " L:");
    OLED_ShowNum(4, 11, Tracking_LineDetected ? 1U : 0U, 1);
}

void Display_Init(void)
{
    OLED_Init();
    OLED_Clear();

#if TRACKING_DISPLAY_MODE
    OLED_ShowString(1, 1, "CH1:----        ");
    OLED_ShowString(2, 1, "CH2:----        ");
    OLED_ShowString(3, 1, "CH3:----        ");
    OLED_ShowString(4, 1, "CH4:----        ");
#else
    OLED_ShowString(1, 1, "L:+0000 C/100ms ");
    OLED_ShowString(2, 1, "R:+0000 C/100ms ");
    OLED_ShowString(3, 1, "S:00000000 C:00 ");
    OLED_ShowString(4, 1, "E:+0000 L:0     ");
#endif
}

void Display_Update(void)
{
    static uint8_t divider = 0;
    static int32_t leftWindowCount = 0;
    static int32_t rightWindowCount = 0;

    leftWindowCount += Encoder_GetLeftSpeed();
    rightWindowCount += Encoder_GetRightSpeed();

    divider++;
    if (divider < DISPLAY_UPDATE_DIVIDER) {
        return;
    }
    divider = 0;

#if TRACKING_DISPLAY_MODE
    Display_ShowCalibration();
#else
    Display_ShowDebugInfo(leftWindowCount, rightWindowCount);
#endif

    leftWindowCount = 0;
    rightWindowCount = 0;
}
