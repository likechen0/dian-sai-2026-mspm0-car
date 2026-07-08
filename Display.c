#include "Display.h"
#include "OLED.h"
#include "Encoder.h"
#include "Tracking.h"
#include "InertialNav.h"

/*
 * OLED 显示模块。
 * 第 1 行：左轮编码器采样速度。
 * 第 2 行：右轮编码器采样速度。
 * 第 3 行：8 路灰度数字状态。
 * 第 4 行：循迹误差和丢线计数。
 */
static uint8_t Display_GetSensorBit(uint8_t index)
{
    uint8_t value = LQ_Tracking_Value[index];

#if !TRACKING_BLACK_IS_HIGH
    /* 保证显示含义固定：1 表示该路传感器检测到线。 */
    value = TRACKING_VALUE_MAX - value;
#endif

    return (value >= TRACKING_LINE_THRESHOLD) ? 1U : 0U;
}

static void Display_ShowSensorBits(void)
{
    char text[17];
    uint8_t i;

    /* 格式化成 S:xxxxxxxx，每个 x 对应一路灰度传感器。 */
    text[0] = 'S';
    text[1] = ':';
    for (i = 0; i < TRACKING_CHANNEL_COUNT; i++) {
        text[i + 2U] = Display_GetSensorBit(i) ? '1' : '0';
    }
    for (i = 10U; i < 16U; i++) {
        text[i] = ' ';
    }
    text[16] = '\0';

    OLED_ShowString(3, 1, text);
}

void Display_Init(void)
{
    OLED_Init();
    OLED_Clear();

    /* 先画固定宽度模板，后续刷新时不容易闪烁和残影。 */
    OLED_ShowString(1, 1, "W1:+00000      ");
    OLED_ShowString(2, 1, "W2:+00000      ");
    OLED_ShowString(3, 1, "S:00000000     ");
    OLED_ShowString(4, 1, "E:+0000 C:0000 ");
}

void Display_Update(void)
{
    static uint8_t divider = 0;

    /* OLED 刷新较慢，所以这里做分频，不是每个控制周期都真正刷新。 */
    divider++;
    if (divider < DISPLAY_UPDATE_DIVIDER) {
        return;
    }
    divider = 0;

    OLED_ShowString(1, 1, "W1:");
    OLED_ShowSignedNum(1, 4, Encoder_GetLeftSpeed(), 5);
    OLED_ShowString(1, 10, "      ");

    OLED_ShowString(2, 1, "W2:");
    OLED_ShowSignedNum(2, 4, Encoder_GetRightSpeed(), 5);
    OLED_ShowString(2, 10, "      ");

    Display_ShowSensorBits();

    /* E 是循迹误差；C 是稳定“有线到没线”的计数。 */
    OLED_ShowString(4, 1, "E:");
    OLED_ShowSignedNum(4, 3, Tracking_Error, 4);
    OLED_ShowString(4, 8, " C:");
    OLED_ShowNum(4, 11, NAV_GetLinePassCount(), 4);
    OLED_ShowString(4, 15, "  ");
}
