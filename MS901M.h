#ifndef MS901M_H
#define MS901M_H

#include <stdbool.h>
#include <stdint.h>

void MS901M_Init(void);
void MS901M_PushByte(uint8_t byte);

bool MS901M_Available(void);
int16_t MS901M_GetYawCdeg(void);
int16_t MS901M_GetRollCdeg(void);
int16_t MS901M_GetPitchCdeg(void);

uint16_t MS901M_GetRxByteCount(void);
uint16_t MS901M_GetAngleFrameCount(void);
uint16_t MS901M_GetBadFrameCount(void);
uint8_t MS901M_GetLastFrameId(void);
uint8_t MS901M_GetLastErrorCode(void);

void MS901M_SetYawZero(void);
int16_t MS901M_YawErrorCdeg(int16_t target, int16_t current);

#endif
