#include "Delay.h"
#include "ti_msp_dl_config.h"

/*
 * Blocking millisecond delay based on the SysConfig CPU clock.
 * Use it only in the main loop or simple drivers; do not call it inside ISRs.
 */
void Delay_ms(uint32_t ms)
{
    while (ms > 0U) {
        delay_cycles(CPUCLK_FREQ / 1000U);
        ms--;
    }
}
