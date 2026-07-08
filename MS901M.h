#ifndef MS901M_H
#define MS901M_H

#include <stdbool.h>
#include <stdint.h>

void MS901M_Init(void);
void MS901M_PushByte(uint8_t byte);

bool MS901M_Available(void);
int16_t MS901M_GetYawCdeg(void);
void MS901M_SetYawZero(void);
int16_t MS901M_YawErrorCdeg(int16_t target, int16_t current);

#endif
