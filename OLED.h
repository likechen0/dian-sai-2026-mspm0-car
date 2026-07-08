#ifndef OLED_H
#define OLED_H

#include <stdint.h>

/* 初始化 SSD1306 OLED，并清屏。 */
void OLED_Init(void);

/* 清空所有像素。 */
void OLED_Clear(void);

/* 显示函数使用从 1 开始的行列坐标：4 行 x 16 列。 */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, const char *String);
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

#endif
