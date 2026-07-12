#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

/* OLED update divider based on the main 10 ms control loop. */
#define DISPLAY_UPDATE_DIVIDER 10U

void Display_Init(void);
void Display_Update(void);

#endif
