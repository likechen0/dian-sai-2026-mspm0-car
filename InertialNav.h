#ifndef INERTIAL_NAV_H
#define INERTIAL_NAV_H

#include <stdint.h>

void NAV_Init(void);
void NAV_ControlStep(void);
void NAV_StartRightTurnCdeg(int16_t angleCdeg);

uint16_t NAV_GetLinePassCount(void);
uint8_t NAV_IsTurning(void);

#endif
