#include "Display.h"
#include "OLED.h"
#include "Encoder.h"
#include "MS901M.h"

/*
 * OLED display:
 * Line 1: left wheel encoder sample count.
 * Line 2: right wheel encoder sample count.
 * Line 3: MS901M yaw and valid-angle-frame state.
 * Line 4: MS901M pitch, roll, and UART byte heartbeat.
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

    if (gyroOk == 0U) {
        OLED_ShowString(3, 1, "RX:");
        OLED_ShowNum(3, 4, MS901M_GetRxByteCount() % 10000U, 4);
        OLED_ShowString(3, 8, " ID:");
        OLED_ShowHexNum(3, 12, MS901M_GetLastFrameId(), 2);
        OLED_ShowString(3, 14, "  ");

        OLED_ShowString(4, 1, "AF:");
        OLED_ShowNum(4, 4, MS901M_GetAngleFrameCount(), 4);
        OLED_ShowString(4, 8, " ER:");
        OLED_ShowNum(4, 12, MS901M_GetLastErrorCode(), 1);
        OLED_ShowString(4, 13, " BD:");
        OLED_ShowNum(4, 16, MS901M_GetBadFrameCount() % 10U, 1);
        return;
    }

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

    OLED_ShowString(1, 1, "W1:+00000      ");
    OLED_ShowString(2, 1, "W2:+00000      ");
    OLED_ShowString(3, 1, "RX:0000 ID:00  ");
    OLED_ShowString(4, 1, "AF:0000 ER:0BD:0");
}

void Display_Update(void)
{
    static uint8_t divider = 0;

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
