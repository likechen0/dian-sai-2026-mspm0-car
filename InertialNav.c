#include "InertialNav.h"
#include "Motor.h"
#include "MS901M.h"
#include "Tracking.h"

/*
 * 导航状态机。
 *
 * 正常行为：
 *   - 没检测到线：按 NAV_FORWARD_SPEED 白地直行。
 *   - 检测到线：用 PD 修正左右轮速度，进行巡线。
 *   - 只有先稳定巡线，再稳定丢线，才计为一次“经过线”。
 *   - 计数为奇数左转 45 度，计数为偶数右转 45 度。
 *
 * 角度单位是 0.01 度：4500 表示 45.00 度。
 *
 * 关键开关位置：
 *   - NAV_LOST_SEARCH_ENABLE：离线追线。丢线后按最后外侧 S1/S8 方向追回。
 *   - NAV_OUTER_GUARD_ENABLE：左右固定转。外侧 S1/S8 识别到线时给固定差速修正。
 *   - NAV_ENABLE_LINE_TURN：脱线导航转。有线到无线后按计数奇偶触发陀螺仪 45 度转向。
 * 0 表示关闭，1 表示打开。调 PID 在 Tracking.h 里改 TRACKING_KP/KD。
 */
#define NAV_FORWARD_SPEED       2000
#define NAV_TURN_45_CDEG        4500
#define NAV_TURN_DONE_CDEG      250
#define NAV_LINE_STABLE_CYCLES  5U
#define NAV_LOST_STABLE_CYCLES  3U
#define NAV_TURN_TIMEOUT_STEPS  200U
/* 脱线导航转开关：改成 1 后，稳定有线 -> 稳定无线会按奇偶计数触发 45 度陀螺仪转向。 */
#define NAV_ENABLE_LINE_TURN    0U

#define NAV_OUTER_LEFT_MASK     ((uint8_t)(1U << 0))
#define NAV_OUTER_RIGHT_MASK    ((uint8_t)(1U << 7))

/* 离线追线开关：改成 1 后，最后只见到 S1/S8 再丢线时，会沿最后外侧方向低速追回。 */
#define NAV_LOST_SEARCH_ENABLE          0U
#define NAV_LOST_SEARCH_LEFT_SIDE       (-1)
#define NAV_LOST_SEARCH_RIGHT_SIDE      1
#define NAV_LOST_SEARCH_FORWARD_SPEED   200
#define NAV_LOST_SEARCH_TURN_SPEED      900

/* 左右固定转开关：改成 1 后，S1 或 S8 单独识别到线时，本周期给固定差速修正。 */
#define NAV_OUTER_GUARD_ENABLE          0U
#define NAV_OUTER_GUARD_SPEED           1400
#define NAV_OUTER_GUARD_CORRECTION      1000

#define NAV_TRACK_MIN_SPEED             1200
#define NAV_TRACK_SLOWDOWN_DEN          4

#define NAV_LEFT_YAW_SIGN       (1)
#define NAV_RIGHT_YAW_SIGN      (-1)
#define NAV_LEFT_LEFT_SPEED     (-1800)
#define NAV_LEFT_RIGHT_SPEED    1800
#define NAV_RIGHT_LEFT_SPEED    1800
#define NAV_RIGHT_RIGHT_SPEED   (-1800)

typedef enum {
    /* 有线状态：由 PD 修正控制左右轮。 */
    NAV_MODE_LINE = 0,

    /* 无线状态：两轮同速，白地直行。 */
    NAV_MODE_FORWARD,
    NAV_MODE_LOST_SEARCH,

    /* 陀螺仪转向状态：原地固定角度转弯。 */
    NAV_MODE_GYRO_TURN
} NavMode_t;

static NavMode_t gMode;
static int16_t gTargetYawCdeg;
static int16_t gTurnLeftSpeed;
static int16_t gTurnRightSpeed;
static int16_t gLastControlError;
static int8_t gLastOuterLineSide;
static int8_t gSearchOuterLineSide;

/* “有线 -> 没线”事件的消抖状态。 */
static uint8_t gLineStableCount;
static uint8_t gLostStableCount;
static uint8_t gLineTurnArmed;

/* 转向超时保护，以及 OLED 上显示的丢线计数。 */
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

static uint16_t NAV_Abs16(int16_t value)
{
    int32_t temp = value;

    if (temp < 0) {
        temp = -temp;
    }

    return (uint16_t)temp;
}

static int16_t NAV_WrapTarget(int32_t target)
{
    /* 把目标角度限制到 MS901M_GetYawCdeg() 使用的同一范围。 */
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
    /* 只清除消抖/允许转向状态，不清除 OLED 上显示的计数。 */
    gLineStableCount = 0;
    gLostStableCount = 0;
    gLineTurnArmed = 0;
}

static uint8_t NAV_UpdateLinePassCounter(void)
{
    uint8_t now = Tracking_LineDetected ? 1U : 0U;

    /*
     * 陀螺仪转向期间忽略灰度边沿。
     * 否则转弯时传感器可能再次扫到同一条线，造成重复计数。
     */
    if ((gMode == NAV_MODE_GYRO_TURN) ||
        (gMode == NAV_MODE_LOST_SEARCH)) {
        return 0U;
    }

    if (now != 0U) {
        /* 必须连续多次检测到线，才允许后续丢线触发转向。 */
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
        /* 如果一开始就在白地上，不算“丢线”。 */
        gLostStableCount = 0;
        return 0U;
    }

    if (gLostStableCount < NAV_LOST_STABLE_CYCLES) {
        gLostStableCount++;
    }

    if (gLostStableCount < NAV_LOST_STABLE_CYCLES) {
        /* 必须连续多次没线，才真正计数。 */
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
    int16_t baseSpeed;

    /* PD 控制：P 跟随位置误差，D 抑制误差突变。 */
    gLastControlError = error;

    correction = ((int32_t)error * TRACKING_KP_NUM) / TRACKING_KP_DEN;
    correction += ((int32_t)derivative * TRACKING_KD_NUM) / TRACKING_KD_DEN;

    Tracking_Correction = NAV_LimitSpeed(correction);
    baseSpeed = NAV_LimitSpeed(
        (int32_t)TRACKING_BASE_SPEED -
        ((int32_t)NAV_Abs16(error) / NAV_TRACK_SLOWDOWN_DEN));
    if (baseSpeed < NAV_TRACK_MIN_SPEED) {
        baseSpeed = NAV_TRACK_MIN_SPEED;
    }

    int16_t leftSpeed =
        NAV_LimitSpeed((int32_t)baseSpeed + Tracking_Correction);
    int16_t rightSpeed =
        NAV_LimitSpeed((int32_t)baseSpeed - Tracking_Correction);

    Motor_SetSpeed(leftSpeed, rightSpeed);
}

#if NAV_OUTER_GUARD_ENABLE
static uint8_t NAV_OuterGuardStep(void)
{
    uint8_t mask = Tracking_GetLineMask();
    uint8_t leftOuter = ((mask & NAV_OUTER_LEFT_MASK) != 0U) ? 1U : 0U;
    uint8_t rightOuter = ((mask & NAV_OUTER_RIGHT_MASK) != 0U) ? 1U : 0U;
    int16_t correction;
    int16_t leftSpeed;
    int16_t rightSpeed;

    /*
     * 外侧修正只给当前周期一个固定差速，不进入强制转向状态。
     * S1 看到线就往左咬，S8 看到线就往右咬。
     */
    if ((leftOuter != 0U) && (rightOuter == 0U)) {
        correction = -NAV_OUTER_GUARD_CORRECTION;
    } else if ((rightOuter != 0U) && (leftOuter == 0U)) {
        correction = NAV_OUTER_GUARD_CORRECTION;
    } else {
        return 0U;
    }

    Tracking_Correction = correction;
    leftSpeed = NAV_LimitSpeed((int32_t)NAV_OUTER_GUARD_SPEED + correction);
    rightSpeed = NAV_LimitSpeed((int32_t)NAV_OUTER_GUARD_SPEED - correction);
    Motor_SetSpeed(leftSpeed, rightSpeed);

    gLastControlError = Tracking_Error;
    return 1U;
}
#endif

static void NAV_UpdateLastOuterLineSide(void)
{
    uint8_t mask = Tracking_GetLineMask();
    uint8_t leftOuter;
    uint8_t rightOuter;

    if (mask == 0U) {
        return;
    }

    leftOuter = ((mask & NAV_OUTER_LEFT_MASK) != 0U) ? 1U : 0U;
    rightOuter = ((mask & NAV_OUTER_RIGHT_MASK) != 0U) ? 1U : 0U;

    if ((leftOuter != 0U) && (rightOuter == 0U)) {
        gLastOuterLineSide = NAV_LOST_SEARCH_LEFT_SIDE;
    } else if ((rightOuter != 0U) && (leftOuter == 0U)) {
        gLastOuterLineSide = NAV_LOST_SEARCH_RIGHT_SIDE;
    } else {
        gLastOuterLineSide = 0;
    }
}

#if NAV_LOST_SEARCH_ENABLE
static void NAV_ClearLostSearchState(void)
{
    gSearchOuterLineSide = 0;
}

static void NAV_LostSearchStep(void)
{
    int16_t turn;
    int16_t leftSpeed;
    int16_t rightSpeed;

    if (Tracking_LineDetected != 0U) {
        gMode = NAV_MODE_LINE;
        NAV_ClearLostSearchState();
        NAV_UpdateLastOuterLineSide();
        gLastControlError = Tracking_Error;
        NAV_LineFollowFromCurrentError();
        return;
    }

    if (gSearchOuterLineSide == 0) {
        gSearchOuterLineSide = gLastOuterLineSide;
    }

    if (gSearchOuterLineSide == 0) {
        gMode = NAV_MODE_FORWARD;
        Tracking_Correction = 0;
        Motor_SetSpeed(NAV_FORWARD_SPEED, NAV_FORWARD_SPEED);
        gLastControlError = Tracking_Error;
        return;
    }

    gMode = NAV_MODE_LOST_SEARCH;
    turn = (int16_t)((int32_t)gSearchOuterLineSide * NAV_LOST_SEARCH_TURN_SPEED);
    leftSpeed = NAV_LimitSpeed((int32_t)NAV_LOST_SEARCH_FORWARD_SPEED + turn);
    rightSpeed = NAV_LimitSpeed((int32_t)NAV_LOST_SEARCH_FORWARD_SPEED - turn);
    Tracking_Correction = turn;
    Motor_SetSpeed(leftSpeed, rightSpeed);
}
#endif

static void NAV_FinishTurn(void)
{
    /* 固定角度转完后，回到白地直行。 */
    gMode = NAV_MODE_FORWARD;
    NAV_ClearLineState();
    Tracking_Correction = 0;
    Motor_SetSpeed(NAV_FORWARD_SPEED, NAV_FORWARD_SPEED);
}

static uint8_t NAV_TurnTimeoutExpired(void)
{
    /* 安全保护：陀螺仪方向或接线错误时，不允许一直原地转。 */
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
    /* 如果陀螺仪没数据，不要卡死在转向状态，优先保持小车可控。 */
    if (Tracking_LineDetected != 0U) {
        gMode = NAV_MODE_LINE;
#if NAV_OUTER_GUARD_ENABLE
        if (NAV_OuterGuardStep() != 0U) {
            return;
        }
#endif
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

    /* 用当前 yaw 加上目标偏移量，得到绝对目标角度。 */
    gTargetYawCdeg = NAV_WrapTarget((int32_t)yaw +
        ((int32_t)yawSign * angleCdeg));
    gTurnLeftSpeed = leftSpeed;
    gTurnRightSpeed = rightSpeed;
    gTurnStepCount = 0;
    NAV_ClearLineState();
    gMode = NAV_MODE_GYRO_TURN;
}

#if NAV_ENABLE_LINE_TURN
static void NAV_StartLeftTurnCdeg(int16_t angleCdeg)
{
    NAV_StartTurnCdeg(angleCdeg, NAV_LEFT_YAW_SIGN,
        NAV_LEFT_LEFT_SPEED, NAV_LEFT_RIGHT_SPEED);
}

static void NAV_StartTurnByLineCount(void)
{
    /* 第 1 次丢线左转，第 2 次丢线右转，后面继续奇左偶右。 */
    if ((gLinePassCount & 1U) != 0U) {
        NAV_StartLeftTurnCdeg(NAV_TURN_45_CDEG);
    } else {
        NAV_StartRightTurnCdeg(NAV_TURN_45_CDEG);
    }
}
#endif

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

    /* 进入目标角度容差范围后，认为转向完成。 */
    if ((error > -NAV_TURN_DONE_CDEG) && (error < NAV_TURN_DONE_CDEG)) {
        NAV_FinishTurn();
        return;
    }

    Motor_SetSpeed(gTurnLeftSpeed, gTurnRightSpeed);
}

void NAV_Init(void)
{
    /* 初始进入安全的直行状态，真正动作由 NAV_ControlStep() 决定。 */
    gMode = NAV_MODE_FORWARD;
    gTargetYawCdeg = 0;
    gTurnLeftSpeed = NAV_RIGHT_LEFT_SPEED;
    gTurnRightSpeed = NAV_RIGHT_RIGHT_SPEED;
    gLastControlError = 0;
    gLastOuterLineSide = 0;
    gSearchOuterLineSide = 0;
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

    /* 先刷新传感器，再根据结果切换状态。 */
    Tracking_Value_Acquire();
    Tracking_CalcError();
    NAV_UpdateLastOuterLineSide();
    lineLost = NAV_UpdateLinePassCounter();

    /* 固定角度转向优先级最高，直到完成或超时。 */
    if (gMode == NAV_MODE_GYRO_TURN) {
        NAV_GyroTurnStep();
        return;
    }

#if NAV_LOST_SEARCH_ENABLE
    if (gMode == NAV_MODE_LOST_SEARCH) {
        NAV_LostSearchStep();
        return;
    }

    if ((Tracking_LineDetected == 0U) && (gLastOuterLineSide != 0)) {
        NAV_LostSearchStep();
        return;
    }
#endif

    if (lineLost != 0U) {
#if NAV_ENABLE_LINE_TURN
        /* 真实稳定的丢线事件，才触发奇偶转向规则。 */
        if (!MS901M_Available()) {
            NAV_FallbackWhenGyroMissing();
            return;
        }

        NAV_StartTurnByLineCount();
        NAV_GyroTurnStep();
#else
        gMode = NAV_MODE_FORWARD;
        Tracking_Correction = 0;
        Motor_SetSpeed(NAV_FORWARD_SPEED, NAV_FORWARD_SPEED);
        gLastControlError = Tracking_Error;
#endif
        return;
    }

    if (Tracking_LineDetected != 0U) {
        /* 普通巡线分支。 */
        gMode = NAV_MODE_LINE;
#if NAV_OUTER_GUARD_ENABLE
        if (NAV_OuterGuardStep() != 0U) {
            return;
        }
#endif
        NAV_LineFollowFromCurrentError();
        return;
    }

    gMode = NAV_MODE_FORWARD;
    Tracking_Correction = 0;
    Motor_SetSpeed(NAV_FORWARD_SPEED, NAV_FORWARD_SPEED);

    /* 无线直行时同步误差，避免重新巡线时 D 项突然变大。 */
    gLastControlError = Tracking_Error;
}

uint16_t NAV_GetLinePassCount(void)
{
    return gLinePassCount;
}

uint8_t NAV_IsTurning(void)
{
    return ((gMode == NAV_MODE_GYRO_TURN) ||
            (gMode == NAV_MODE_LOST_SEARCH)) ? 1U : 0U;
}
