#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

/*
 * 循迹显示模式（编译期切换，仅在调试模式下生效，改完重新编译烧录）。
 *
 *   0 = 01 检查模式
 *       OLED 第 3 行 S: 显示八路 0/1 线状态，
 *       第 4 行显示跟踪误差 E: 和是否有线 L:。
 *       用于检查每一路传感器是否能正确区分黑白：
 *         放白底上 → 对应位应为 0
 *         放黑线上 → 对应位应为 1
 *       如果白底误判为 1 → 增大 Tracking.h 中的 TRACKING_LINE_THRESHOLD
 *       如果黑线误判为 0 → 减小 Tracking.h 中的 TRACKING_LINE_THRESHOLD
 *
 *   1 = ADC 获取模式
 *       OLED 显示八路原始 ADC 值（4 行 × 每行 2 路）。
 *       用于手工采集黑白标定值：
 *         第 1 步：传感器放白色地面上 → 目视记录 8 路 ADC 值
 *                 → 写入 Tracking.c 的 Tracking_CalWhite[] 数组
 *         第 2 步：传感器放黑色线上 → 目视记录 8 路 ADC 值
 *                 → 写入 Tracking.c 的 Tracking_CalBlack[] 数组
 *         第 3 步：改回 0，重新编译烧录，进入 01 检查模式验证
 *                 → 确认黑白区分正常后即可切到巡线模式
 */
#define TRACKING_DISPLAY_MODE  0U

/* OLED update divider based on the main 10 ms control loop. */
#define DISPLAY_UPDATE_DIVIDER 10U

void Display_Init(void);
void Display_Update(void);

#endif
