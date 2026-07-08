#ifndef INERTIAL_NAV_H
#define INERTIAL_NAV_H

#include <stdint.h>

/* Initialize navigation state and the MS901M receiver. */
void NAV_Init(void);

/* One control step: read line sensors, run state machine, command motors. */
void NAV_ControlStep(void);

/* Manual helper for starting a right turn. angleCdeg: 4500 = 45.00 deg. */
void NAV_StartRightTurnCdeg(int16_t angleCdeg);

/* Count of stable line-leave events, shown on OLED as C. */
uint16_t NAV_GetLinePassCount(void);

/* Nonzero while the car is executing a gyro-based turn. */
uint8_t NAV_IsTurning(void);

#endif
