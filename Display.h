#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

/* OLED 刷新分频系数，基准是主循环 10ms 周期。 */
#define DISPLAY_UPDATE_DIVIDER 10U

/* 初始化外接 I2C OLED，并画固定显示模板。 */
void Display_Init(void);

/* 刷新轮速、灰度状态、循迹误差和计数。 */
void Display_Update(void);

#endif
