#include "Delay.h"
#include "ti_msp_dl_config.h"

/*
 * 基于 SysConfig 配置的 CPU 时钟做阻塞毫秒延时。
 * 只适合在主循环或简单驱动里用，不要放进中断函数。
 */
void Delay_ms(uint32_t ms)
{
    while (ms > 0U) {
        delay_cycles(CPUCLK_FREQ / 1000U);
        ms--;
    }
}
