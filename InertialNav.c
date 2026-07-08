#include "InertialNav.h"
#include "Motor.h"
#include "MS901M.h"
#include "Tracking.h"

/*
 * Navigation state machine.
 *
 * Normal behavior:
 *   - If no line is detected, drive straight at NAV_FORWARD_SPEED.
 *   - If a line is detected, follow it with PD correction.
 *   - Only after the car has followed a stable line, a stable line-loss event
 *     is counted. Odd counts turn left 45 degrees; even counts turn right.
 *
 * Angles are centidegrees: 4500 means 45.00 degrees.
 */
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
    /* Line is visible and PD correction is controlling the motors. */
    NAV_MODE_LINE = 0,

    /* No line is visible, so both wheels run at the same default speed. */
    NAV_MODE_FORWARD,

    /* Gyro is controlling an in-place fixed-angle turn. */
    NAV_MODE_GYRO_TURN
} NavMode_t;

static NavMode_t gMode;
static int16_t gTargetYawCdeg;
static int16_t gTurnLeftSpeed;
static int16_t gTurnRightSpeed;
static int16_t gLastControlError;

/* Debounce state for "line -> no line" events. */
static uint8_t gLineStableCount;
static uint8_t gLostStableCount;
static uint8_t gLineTurnArmed;

/* Turn watchdog and OLED-visible line-leave counter. */
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
    /* Keep target yaw inside the same range used by MS901M_GetYawCdeg(). */
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
    /* Clear only the debounce/arming state, not the visible pass counter. */
    gLineStableCount = 0;
    gLostStableCount = 0;
    gLineTurnArmed = 0;
}

static uint8_t NAV_UpdateLinePassCounter(void)
{
    uint8_t now = Tracking_LineDetected ? 1U : 0U;

    /*
     * Ignore line edges during a gyro turn. Otherwise the sensor can see the
     * same line while rotating and count it again.
     */
    if (gMode == NAV_MODE_GYRO_TURN) {
        return 0U;
    }

    if (now != 0U) {
        /* Require several consecutive line readings before arming a turn. */
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
        /* Starting on white ground does not count as losing a line. */
        gLostStableCount = 0;
        return 0U;
    }

    if (gLostStableCount < NAV_LOST_STABLE_CYCLES) {
        gLostStableCount++;
    }

    if (gLostStableCount < NAV_LOST_STABLE_CYCLES) {
        /* Require several consecutive no-line readings before counting. */
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

    /* PD controller: P follows position error, D damps sudden error changes. */
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
    /* After a fixed-angle turn, resume straight driving on white ground. */
    gMode = NAV_MODE_FORWARD;
    NAV_ClearLineState();
    Tracking_Correction = 0;
    Motor_SetSpeed(NAV_FORWARD_SPEED, NAV_FORWARD_SPEED);
}

static uint8_t NAV_TurnTimeoutExpired(void)
{
    /* Safety: do not spin forever if gyro direction or wiring is wrong. */
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
    /* If gyro data is missing, keep the car usable instead of locking in turn. */
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

    /* Build an absolute target yaw from the current yaw and desired offset. */
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
    /* First counted line-loss: left. Second: right. Then repeat. */
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

    /* Stop turning once the target is reached within the tolerance window. */
    if ((error > -NAV_TURN_DONE_CDEG) && (error < NAV_TURN_DONE_CDEG)) {
        NAV_FinishTurn();
        return;
    }

    Motor_SetSpeed(gTurnLeftSpeed, gTurnRightSpeed);
}

void NAV_Init(void)
{
    /* Start in safe forward mode; movement is commanded by NAV_ControlStep(). */
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

    /* Sensor processing happens before state decisions. */
    Tracking_Value_Acquire();
    Tracking_CalcError();
    lineLost = NAV_UpdateLinePassCounter();

    /* Fixed-angle turn has the highest priority until it finishes or times out. */
    if (gMode == NAV_MODE_GYRO_TURN) {
        NAV_GyroTurnStep();
        return;
    }

    if (lineLost != 0U) {
        /* A real line-leave event triggers the odd/even turn rule. */
        if (!MS901M_Available()) {
            NAV_FallbackWhenGyroMissing();
            return;
        }

        NAV_StartTurnByLineCount();
        NAV_GyroTurnStep();
        return;
    }

    if (Tracking_LineDetected != 0U) {
        /* Ordinary line-following branch. */
        gMode = NAV_MODE_LINE;
        NAV_LineFollowFromCurrentError();
        return;
    }

    gMode = NAV_MODE_FORWARD;
    Tracking_Correction = 0;
    Motor_SetSpeed(NAV_FORWARD_SPEED, NAV_FORWARD_SPEED);

    /* Keep D-term calm when re-entering line-follow mode later. */
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
