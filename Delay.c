#include "Delay.h"
#include "ti_msp_dl_config.h"

void Delay_ms(uint32_t ms)
{
    while (ms > 0U) {
        delay_cycles(CPUCLK_FREQ / 1000U);
        ms--;
    }
}
