#include "Display.h"
#include "OLED.h"
#include "Encoder.h"
#include "Tracking.h"
#include "InertialNav.h"

static uint8_t Display_GetSensorBit(uint8_t index)
{
    uint8_t value = LQ_Tracking_Value[index];

#if !TRACKING_BLACK_IS_HIGH
    value = TRACKING_VALUE_MAX - value;
#endif

    return (value >= TRACKING_LINE_THRESHOLD) ? 1U : 0U;
}

static void Display_ShowSensorBits(void)
{
    char text[17];
    uint8_t i;

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

    OLED_ShowString(1, 1, "W1:+00000      ");
    OLED_ShowString(2, 1, "W2:+00000      ");
    OLED_ShowString(3, 1, "S:00000000     ");
    OLED_ShowString(4, 1, "E:+0000 C:0000 ");
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

    Display_ShowSensorBits();

    OLED_ShowString(4, 1, "E:");
    OLED_ShowSignedNum(4, 3, Tracking_Error, 4);
    OLED_ShowString(4, 8, " C:");
    OLED_ShowNum(4, 11, NAV_GetLinePassCount(), 4);
    OLED_ShowString(4, 15, "  ");
}
