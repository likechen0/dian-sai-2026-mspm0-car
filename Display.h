#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

/* Display refresh divider relative to the main 10 ms control loop. */
#define DISPLAY_UPDATE_DIVIDER 10U

/* Initialize the external I2C OLED and draw the fixed text template. */
void Display_Init(void);

/* Refresh wheel speeds, grayscale bits, tracking error, and pass count. */
void Display_Update(void);

#endif
