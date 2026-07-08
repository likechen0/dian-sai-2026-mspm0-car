#include "AD.h"
#include "Tracking.h"

void AD_Init(void)
{
}

uint16_t AD_GetValue(uint8_t ADC_Channel)
{
    (void)ADC_Channel;
    return Tracking_Adc_once();
}
