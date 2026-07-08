#include "FIND.h"
#include "AD.h"
#include "Motor.h"
#include "ti_msp_dl_config.h"

uint16_t AD_L, AD_R;
float Error;
float Error_Last;
float Kp = 0.7f, Ki = 0.0f, Kd = 0.0f;
float dt = 0.001f;
float Eet;

float out;
int16_t Speed_L;
int16_t Speed_R;

#define BASE_SPEED (5000)
#define TURN_GAIN (0.25f)
#define SPEED_LIMIT (10000.0f)
#define INTEGRAL_LIMIT (40000.0f)

static int16_t FIND_ClampSpeed(float speed)
{
    if (speed > SPEED_LIMIT) {
        return (int16_t) SPEED_LIMIT;
    }

    if (speed < -SPEED_LIMIT) {
        return (int16_t) -SPEED_LIMIT;
    }

    return (int16_t) speed;
}

void FIND_GO(void)
{
    AD_L = AD_GetValue(AD_CHANNEL_LEFT);
    AD_R = AD_GetValue(AD_CHANNEL_RIGHT);

    Error = (float) (AD_L - AD_R) * 0.01f;
    Error = Error / dt;
    Eet = Eet + Error * dt;

    if (Eet > INTEGRAL_LIMIT) {
        Eet = INTEGRAL_LIMIT;
    } else if (Eet < -INTEGRAL_LIMIT) {
        Eet = -INTEGRAL_LIMIT;
    }

    out = Kp * Error + Ki * Eet + Kd * (Error - Error_Last) / dt;
    Error_Last = Error;

    if ((Error > 30000.0f) || (Error < -30000.0f)) {
        out = out * 1.2f;
    }

    float turn = out * TURN_GAIN;

    Speed_L = FIND_ClampSpeed((float) BASE_SPEED - turn);
    Speed_R = FIND_ClampSpeed((float) BASE_SPEED + turn);

    Motor_SetSpeed_L(Speed_L);
    Motor_SetSpeed_R(Speed_R);
    delay_cycles(CPUCLK_FREQ / 1000U);
}
