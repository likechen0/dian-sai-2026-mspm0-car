#include "Display.h"
#include "OLED.h"
#include "Encoder.h"
#include "MS901M.h"

/*
 * OLED 显示模块。
 * 第 1 行：左轮编码器采样速度。
 * 第 2 行：右轮编码器采样速度。
 * 第 3 行：MS901M yaw 航向角和数据有效状态。
 * 第 4 行：MS901M pitch / roll 姿态角。
 */
static uint16_t Display_AbsCdeg(int16_t value)
{
    int32_t temp = value;

    if (temp < 0) {
        temp = -temp;
    }

    return (uint16_t)temp;
}

static void Display_ShowSignedCdeg(uint8_t line, uint8_t column, int16_t value)
{
    uint16_t absValue = Display_AbsCdeg(value);

    OLED_ShowChar(line, column, (value < 0) ? '-' : '+');
    OLED_ShowNum(line, column + 1U, absValue / 100U, 3);
    OLED_ShowChar(line, column + 4U, '.');
    OLED_ShowNum(line, column + 5U, absValue % 100U, 2);
}

static void Display_ShowSignedDeg(uint8_t line, uint8_t column, int16_t value)
{
    uint16_t absValue = Display_AbsCdeg(value);

    OLED_ShowChar(line, column, (value < 0) ? '-' : '+');
    OLED_ShowNum(line, column + 1U, absValue / 100U, 3);
}

static void Display_ShowGyro(void)
{
    uint8_t gyroOk = MS901M_Available() ? 1U : 0U;

    OLED_ShowString(3, 1, "Y:");
    Display_ShowSignedCdeg(3, 3, MS901M_GetYawCdeg());
    OLED_ShowString(3, 10, " OK:");
    OLED_ShowNum(3, 14, gyroOk, 1);
    OLED_ShowString(3, 15, "  ");

    OLED_ShowString(4, 1, "P:");
    Display_ShowSignedDeg(4, 3, MS901M_GetPitchCdeg());
    OLED_ShowString(4, 7, " R:");
    Display_ShowSignedDeg(4, 10, MS901M_GetRollCdeg());
    OLED_ShowString(4, 14, "   ");
}

void Display_Init(void)
{
    OLED_Init();
    OLED_Clear();

    /* 先画固定宽度模板，后续刷新时不容易闪烁和残影。 */
    OLED_ShowString(1, 1, "W1:+00000      ");
    OLED_ShowString(2, 1, "W2:+00000      ");
    OLED_ShowString(3, 1, "Y:+000.00 OK:0 ");
    OLED_ShowString(4, 1, "P:+000 R:+000  ");
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

    Display_ShowGyro();
}
