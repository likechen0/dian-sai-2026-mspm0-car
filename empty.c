#include "ti_msp_dl_config.h"
#include "Motor.h"
#include "Encoder.h"
#include "Delay.h"
#include "Tracking.h"
#include "Display.h"
#include "InertialNav.h"

int main(void)
{
    SYSCFG_DL_init();

    Motor_Init();
    Encoder_Init();
    Tracking_Init();
    NAV_Init();
    Display_Init();

    while (1) {
        NAV_ControlStep();
        Delay_ms(ENCODER_SAMPLE_MS);
        Encoder_Update();
        Display_Update();

        DL_GPIO_togglePins(RUN_LED_PORT, RUN_LED_LED_PIN);
    }
}
