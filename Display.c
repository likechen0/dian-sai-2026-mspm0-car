#include "Display.h"
#include "OLED.h"
#include "Encoder.h"
#include "Tracking.h"

/*
 * OLED display:
 * Line 1: left wheel encoder speed, pulses per ENCODER_SAMPLE_MS.
 * Line 2: right wheel encoder speed, pulses per ENCODER_SAMPLE_MS.
 * Line 3: sensor detection mask, S1 to S8 from left to right.
 * Line 4: tracking error and line-detected flag.
 */
static void Display_ShowSensorMask(uint8_t line, uint8_t column, uint8_t mask)
{
    uint8_t i;

    for (i = 0U; i < TRACKING_CHANNEL_COUNT; i++) {
        OLED_ShowChar(line, (uint8_t)(column + i),
            ((mask & (uint8_t)(1U << i)) != 0U) ? '1' : '0');
    }
}

static void Display_ShowDebugInfo(void)
{
    OLED_ShowString(1, 1, "Lspd:");
    OLED_ShowSignedNum(1, 6, Encoder_GetLeftSpeed(), 4);
    OLED_ShowString(1, 11, "      ");

    OLED_ShowString(2, 1, "Rspd:");
    OLED_ShowSignedNum(2, 6, Encoder_GetRightSpeed(), 4);
    OLED_ShowString(2, 11, "      ");

    OLED_ShowString(3, 1, "S:");
    Display_ShowSensorMask(3, 3, Tracking_GetLineMask());
    OLED_ShowString(3, 11, " 1-8 ");

    OLED_ShowString(4, 1, "E:");
    OLED_ShowSignedNum(4, 3, Tracking_Error, 4);
    OLED_ShowString(4, 8, " Line:");
    OLED_ShowNum(4, 14, Tracking_LineDetected ? 1U : 0U, 1);
    OLED_ShowString(4, 15, "  ");
}

void Display_Init(void)
{
    OLED_Init();
    OLED_Clear();

    OLED_ShowString(1, 1, "Lspd:+0000      ");
    OLED_ShowString(2, 1, "Rspd:+0000      ");
    OLED_ShowString(3, 1, "S:00000000 1-8 ");
    OLED_ShowString(4, 1, "E:+0000 Line:0 ");
}

void Display_Update(void)
{
    static uint8_t divider = 0;

    divider++;
    if (divider < DISPLAY_UPDATE_DIVIDER) {
        return;
    }
    divider = 0;

    Display_ShowDebugInfo();
}
