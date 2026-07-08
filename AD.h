#ifndef AD_H
#define AD_H

#include <stdint.h>

#define AD_CHANNEL_LEFT  0U
#define AD_CHANNEL_RIGHT 1U

void AD_Init(void);
uint16_t AD_GetValue(uint8_t ADC_Channel);

#endif
