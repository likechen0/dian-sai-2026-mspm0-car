#include "Display.h"
#include "OLED.h"
#include "Encoder.h"
#include "MS901M.h"

/*
 * OLED display:
 * Line 1: odometry X.
 * Line 2: odometry Y.
 * Line 3: heading angle.
 * Line 4: left/right accumulated encoder position.
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

static void Display_ShowPosition(void)
{
    OLED_ShowString(1, 1, "X:");
    OLED_ShowSignedNum(1, 3, Encoder_GetXmm(), 6);
    OLED_ShowString(1, 10, "mm     ");

    OLED_ShowString(2, 1, "Y:");
    OLED_ShowSignedNum(2, 3, Encoder_GetYmm(), 6);
    OLED_ShowString(2, 10, "mm     ");

    OLED_ShowString(3, 1, "A:");
    Display_ShowSignedCdeg(3, 3, Encoder_GetPoseYawCdeg());
    OLED_ShowString(3, 10, " OK:");
    OLED_ShowNum(3, 14, MS901M_Available() ? 1U : 0U, 1);
    OLED_ShowString(3, 15, "  ");

    OLED_ShowString(4, 1, "L:");
    OLED_ShowSignedNum(4, 3, Encoder_GetLeftTotalCount(), 4);
    OLED_ShowString(4, 8, " R:");
    OLED_ShowSignedNum(4, 11, Encoder_GetRightTotalCount(), 4);
    OLED_ShowChar(4, 16, ' ');
}

void Display_Init(void)
{
    OLED_Init();
    OLED_Clear();

    OLED_ShowString(1, 1, "X:+000000mm    ");
    OLED_ShowString(2, 1, "Y:+000000mm    ");
    OLED_ShowString(3, 1, "A:+000.00 OK:0 ");
    OLED_ShowString(4, 1, "L:+0000 R:+0000");
}

void Display_Update(void)
{
    static uint8_t divider = 0;

    divider++;
    if (divider < DISPLAY_UPDATE_DIVIDER) {
        return;
    }
    divider = 0;

    Display_ShowPosition();
}
