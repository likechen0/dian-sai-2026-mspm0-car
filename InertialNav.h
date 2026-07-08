#ifndef INERTIAL_NAV_H
#define INERTIAL_NAV_H

#include <stdint.h>

/* 初始化导航状态机和 MS901M 接收模块。 */
void NAV_Init(void);

/* 执行一次控制：读灰度、跑状态机、给电机下命令。 */
void NAV_ControlStep(void);

/* 手动启动右转。angleCdeg 单位是 0.01 度，4500 = 45.00 度。 */
void NAV_StartRightTurnCdeg(int16_t angleCdeg);

/* 稳定丢线计数，OLED 上显示为 C。 */
uint16_t NAV_GetLinePassCount(void);

/* 非 0 表示正在执行陀螺仪转向。 */
uint8_t NAV_IsTurning(void);

#endif
