#ifndef OLED_H
#define OLED_H

#include <stdint.h>

/* Initialize the SSD1306 OLED and clear the screen. */
void OLED_Init(void);

/* Clear all pixels. */
void OLED_Clear(void);

/* Display helpers use 1-based line/column coordinates: 4 lines x 16 columns. */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, const char *String);
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

#endif
