#ifndef MS901M_H
#define MS901M_H

#include <stdbool.h>
#include <stdint.h>

/* Initialize UART interrupt state for MS901M angle reception. */
void MS901M_Init(void);

/* Push one received UART byte into the frame parser. Usually called by ISR. */
void MS901M_PushByte(uint8_t byte);

/* True after one valid yaw frame has been decoded. */
bool MS901M_Available(void);

/* Current yaw in centidegrees: 4500 means 45.00 degrees. */
int16_t MS901M_GetYawCdeg(void);

/* Treat the current yaw as 0 degrees for later relative turns. */
void MS901M_SetYawZero(void);

/* Shortest signed yaw error from current to target, in centidegrees. */
int16_t MS901M_YawErrorCdeg(int16_t target, int16_t current);

#endif
