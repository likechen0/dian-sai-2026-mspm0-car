#include "Display.h"
#include "OLED.h"
#include "Encoder.h"
#include "Tracking.h"
#include "MS901M.h"
#include "InertialNav.h"

/*
 * 显示模式由 Display.h 中的 TRACKING_DISPLAY_MODE 宏控制：
 *   0 = 01 检查模式：显示八路 0/1 线状态 + 误差
 *   1 = ADC 获取模式：显示八路原始 ADC 值
 * 详见 Display.h 头注释。
 */

#if !TRACKING_DISPLAY_MODE
static void Display_ShowSensorMask(uint8_t line, uint8_t column, uint8_t mask)
{
    uint8_t i;

    for (i = 0U; i < TRACKING_CHANNEL_COUNT; i++) {
        OLED_ShowChar(line, (uint8_t)(column + i),
            ((mask & (uint8_t)(1U << i)) != 0U) ? '1' : '0');
    }
}
#endif

/*
 * 校准显示：一页显示 8 路原始 ADC。
 * 每行两路，4 行刚好 8 路。
 * 格式：1:1234 2:1234
 */
#if TRACKING_DISPLAY_MODE
static void Display_ShowCalibration(void)
{
    uint8_t line;

    for (line = 0U; line < 4U; line++) {
        uint8_t chL = (uint8_t)(line * 2U + 1U);  /* 左列通道 1,3,5,7 */
        uint8_t chR = (uint8_t)(line * 2U + 2U);  /* 右列通道 2,4,6,8 */

        /* 左列：通道号 + 4位ADC值，占8列 */
        OLED_ShowNum((uint8_t)(line + 1U), 1, chL, 1);
        OLED_ShowChar((uint8_t)(line + 1U), 2, ':');
        OLED_ShowNum((uint8_t)(line + 1U), 3, Ganv_Tracking_Raw[chL - 1U], 4);

        /* 右列：通道号 + 4位ADC值，占8列 */
        OLED_ShowNum((uint8_t)(line + 1U), 9, chR, 1);
        OLED_ShowChar((uint8_t)(line + 1U), 10, ':');
        OLED_ShowNum((uint8_t)(line + 1U), 11, Ganv_Tracking_Raw[chR - 1U], 4);
    }
}
#endif

/*
 * 陀螺仪 YPR + 巡线显示：
 *   行1: Yaw + Pitch (角度值, 1位小数)
 *   行2: Roll + Available
 *   行3: 传感器 LineMask + 丢线计数
 *   行4: 跟踪误差 + 是否有线
 */

/* 将 0.01° 转为 0.1° 并在指定位置显示 (格式: ±xxx.x) */
#if !TRACKING_DISPLAY_MODE
static void Display_ShowAngle(uint8_t line, uint8_t col, int32_t cdeg)
{
    int32_t  tdeg = cdeg / 10;                       /* 0.01° → 0.1° */
    uint32_t abs  = (tdeg >= 0) ? (uint32_t)tdeg : (uint32_t)(-tdeg);
    uint32_t ip   = abs / 10U;                       /* 整数部分 */
    uint32_t dp   = abs % 10U;                       /* 小数部分 (1位) */

    OLED_ShowChar(line, col, (tdeg >= 0) ? '+' : '-');
    OLED_ShowNum(line, (uint8_t)(col + 1U), ip, 3);
    OLED_ShowChar(line, (uint8_t)(col + 4U), '.');
    OLED_ShowNum(line, (uint8_t)(col + 5U), dp, 1);
}

static void Display_ShowDebugInfo(void)
{
    /* 行1: Yaw + Pitch */
    OLED_ShowString(1, 1, "Y:");
    Display_ShowAngle(1, 3, MS901M_GetYawCdeg());
    OLED_ShowString(1, 9, " P:");
    Display_ShowAngle(1, 12, MS901M_GetPitchCdeg());

    /* 行2: Roll + 陀螺仪数据是否有效 */
    OLED_ShowString(2, 1, "R:");
    Display_ShowAngle(2, 3, MS901M_GetRollCdeg());
    OLED_ShowString(2, 9, " A:");
    OLED_ShowNum(2, 12, MS901M_Available() ? 1U : 0U, 1);

    /* 行3: 传感器 + 丢线计数 */
    OLED_ShowString(3, 1, "S:");
    Display_ShowSensorMask(3, 3, Tracking_GetLineMask());
    OLED_ShowString(3, 11, " C:");
    OLED_ShowNum(3, 14, NAV_GetLinePassCount(), 2);

    /* 行4: 跟踪误差 + 是否有线 */
    OLED_ShowString(4, 1, "E:");
    OLED_ShowSignedNum(4, 3, Tracking_Error, 4);
    OLED_ShowString(4, 8, " L:");
    OLED_ShowNum(4, 11, Tracking_LineDetected ? 1U : 0U, 1);
}
#endif

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
    OLED_ShowString(1, 1, "Y:+  0.0 P:+ 0.0");
    OLED_ShowString(2, 1, "R:+  0.0 A:0    ");
    OLED_ShowString(3, 1, "S:00000000 C:00 ");
    OLED_ShowString(4, 1, "E:+0000 L:0     ");
#endif
}

void Display_Update(void)
{
#if TRACKING_DISPLAY_MODE
    Display_ShowCalibration();
#else
    Display_ShowDebugInfo();
#endif
}
