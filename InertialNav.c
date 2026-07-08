#include "InertialNav.h"
#include "Motor.h"
#include "MS901M.h"
#include "Tracking.h"

#define NAV_FORWARD_SPEED       2500
#define NAV_TURN_45_CDEG        4500
#define NAV_TURN_DONE_CDEG      250
#define NAV_LINE_STABLE_CYCLES  5U
#define NAV_LOST_STABLE_CYCLES  3U
#define NAV_TURN_TIMEOUT_STEPS  200U

#define NAV_LEFT_YAW_SIGN       (1)
#define NAV_RIGHT_YAW_SIGN      (-1)
#define NAV_LEFT_LEFT_SPEED     (-1800)
#define NAV_LEFT_RIGHT_SPEED    1800
#define NAV_RIGHT_LEFT_SPEED    1800
#define NAV_RIGHT_RIGHT_SPEED   (-1800)

typedef enum {
    NAV_MODE_LINE = 0,
    NAV_MODE_FORWARD,
    NAV_MODE_GYRO_TURN
} NavMode_t;

static NavMode_t gMode;
static int16_t gTargetYawCdeg;
static int16_t gTurnLeftSpeed;
static int16_t gTurnRightSpeed;
static int16_t gLastControlError;
static uint8_t gLineStableCount;
static uint8_t gLostStableCount;
static uint8_t gLineTurnArmed;
static uint16_t gTurnStepCount;
static uint16_t gLinePassCount;

static int16_t NAV_LimitSpeed(int32_t speed)
{
    if (speed > MOTOR_SPEED_MAX) {
        return MOTOR_SPEED_MAX;
    }
    if (speed < -MOTOR_SPEED_MAX) {
        return -MOTOR_SPEED_MAX;
    }

    return (int16_t)speed;
}

static int16_t NAV_WrapTarget(int32_t target)
{
    while (target > 18000) {
        target -= 36000;
    }
    while (target < -18000) {
        target += 36000;
    }

    return (int16_t)target;
}

static void NAV_ClearLineState(void)
{
    gLineStableCount = 0;
    gLostStableCount = 0;
    gLineTurnArmed = 0;
}

static uint8_t NAV_UpdateLinePassCounter(void)
{
    uint8_t now = Tracking_LineDetected ? 1U : 0U;

    if (gMode == NAV_MODE_GYRO_TURN) {
        return 0U;
    }

    if (now != 0U) {
        if (gLineStableCount < NAV_LINE_STABLE_CYCLES) {
            gLineStableCount++;
        }
        if (gLineStableCount >= NAV_LINE_STABLE_CYCLES) {
            gLineTurnArmed = 1U;
        }

        gLostStableCount = 0;
        return 0U;
    }

    gLineStableCount = 0;

    if (gLineTurnArmed == 0U) {
        gLostStableCount = 0;
        return 0U;
    }

    if (gLostStableCount < NAV_LOST_STABLE_CYCLES) {
        gLostStableCount++;
    }

    if (gLostStableCount < NAV_LOST_STABLE_CYCLES) {
        return 0U;
    }

    if (gLinePassCount < 9999U) {
        gLinePassCount++;
    }

    NAV_ClearLineState();
    return 1U;
}

static void NAV_LineFollowFromCurrentError(void)
{
    int16_t error = Tracking_Error;
    int16_t derivative = error - gLastControlError;
    int32_t correction;

    gLastControlError = error;

    correction = ((int32_t)error * TRACKING_KP_NUM) / TRACKING_KP_DEN;
    correction += ((int32_t)derivative * TRACKING_KD_NUM) / TRACKING_KD_DEN;

    Tracking_Correction = NAV_LimitSpeed(correction);

    int16_t leftSpeed =
        NAV_LimitSpeed((int32_t)TRACKING_BASE_SPEED + Tracking_Correction);
    int16_t rightSpeed =
        NAV_LimitSpeed((int32_t)TRACKING_BASE_SPEED - Tracking_Correction);

    Motor_SetSpeed(leftSpeed, rightSpeed);
}

static void NAV_FinishTurn(void)
{
    gMode = NAV_MODE_FORWARD;
    NAV_ClearLineState();
    Tracking_Correction = 0;
    Motor_SetSpeed(NAV_FORWARD_SPEED, NAV_FORWARD_SPEED);
}

static uint8_t NAV_TurnTimeoutExpired(void)
{
    if (gTurnStepCount < NAV_TURN_TIMEOUT_STEPS) {
        gTurnStepCount++;
    }

    if (gTurnStepCount >= NAV_TURN_TIMEOUT_STEPS) {
        NAV_FinishTurn();
        return 1U;
    }

    return 0U;
}

static void NAV_FallbackWhenGyroMissing(void)
{
    if (Tracking_LineDetected != 0U) {
        gMode = NAV_MODE_LINE;
        NAV_LineFollowFromCurrentError();
        return;
    }

    gMode = NAV_MODE_FORWARD;
    Tracking_Correction = 0;
    Motor_SetSpeed(NAV_FORWARD_SPEED, NAV_FORWARD_SPEED);
}

static void NAV_AbortTurnToCurrentMode(void)
{
    NAV_ClearLineState();
    NAV_FallbackWhenGyroMissing();
}

static void NAV_StartTurnCdeg(int16_t angleCdeg, int8_t yawSign,
                              int16_t leftSpeed, int16_t rightSpeed)
{
    int16_t yaw = MS901M_GetYawCdeg();

    gTargetYawCdeg = NAV_WrapTarget((int32_t)yaw +
        ((int32_t)yawSign * angleCdeg));
    gTurnLeftSpeed = leftSpeed;
    gTurnRightSpeed = rightSpeed;
    gTurnStepCount = 0;
    NAV_ClearLineState();
    gMode = NAV_MODE_GYRO_TURN;
}

static void NAV_StartLeftTurnCdeg(int16_t angleCdeg)
{
    NAV_StartTurnCdeg(angleCdeg, NAV_LEFT_YAW_SIGN,
        NAV_LEFT_LEFT_SPEED, NAV_LEFT_RIGHT_SPEED);
}

static void NAV_StartTurnByLineCount(void)
{
    if ((gLinePassCount & 1U) != 0U) {
        NAV_StartLeftTurnCdeg(NAV_TURN_45_CDEG);
    } else {
        NAV_StartRightTurnCdeg(NAV_TURN_45_CDEG);
    }
}

static void NAV_GyroTurnStep(void)
{
    if (!MS901M_Available()) {
        NAV_AbortTurnToCurrentMode();
        return;
    }

    if (NAV_TurnTimeoutExpired() != 0U) {
        return;
    }

    int16_t yaw = MS901M_GetYawCdeg();
    int16_t error = MS901M_YawErrorCdeg(gTargetYawCdeg, yaw);

    if ((error > -NAV_TURN_DONE_CDEG) && (error < NAV_TURN_DONE_CDEG)) {
        NAV_FinishTurn();
        return;
    }

    Motor_SetSpeed(gTurnLeftSpeed, gTurnRightSpeed);
}

void NAV_Init(void)
{
    gMode = NAV_MODE_FORWARD;
    gTargetYawCdeg = 0;
    gTurnLeftSpeed = NAV_RIGHT_LEFT_SPEED;
    gTurnRightSpeed = NAV_RIGHT_RIGHT_SPEED;
    gLastControlError = 0;
    gTurnStepCount = 0;
    gLinePassCount = 0;
    NAV_ClearLineState();

    MS901M_Init();
}

void NAV_StartRightTurnCdeg(int16_t angleCdeg)
{
    NAV_StartTurnCdeg(angleCdeg, NAV_RIGHT_YAW_SIGN,
        NAV_RIGHT_LEFT_SPEED, NAV_RIGHT_RIGHT_SPEED);
}

void NAV_ControlStep(void)
{
    uint8_t lineLost;

    Tracking_Value_Acquire();
    Tracking_CalcError();
    lineLost = NAV_UpdateLinePassCounter();

    if (gMode == NAV_MODE_GYRO_TURN) {
        NAV_GyroTurnStep();
        return;
    }

    if (lineLost != 0U) {
        if (!MS901M_Available()) {
            NAV_FallbackWhenGyroMissing();
            return;
        }

        NAV_StartTurnByLineCount();
        NAV_GyroTurnStep();
        return;
    }

    if (Tracking_LineDetected != 0U) {
        gMode = NAV_MODE_LINE;
        NAV_LineFollowFromCurrentError();
        return;
    }

    gMode = NAV_MODE_FORWARD;
    Tracking_Correction = 0;
    Motor_SetSpeed(NAV_FORWARD_SPEED, NAV_FORWARD_SPEED);

    gLastControlError = Tracking_Error;
}

uint16_t NAV_GetLinePassCount(void)
{
    return gLinePassCount;
}

uint8_t NAV_IsTurning(void)
{
    return (gMode == NAV_MODE_GYRO_TURN) ? 1U : 0U;
}
