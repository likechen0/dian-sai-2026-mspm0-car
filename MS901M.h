#ifndef MS901M_H
#define MS901M_H

#include <stdbool.h>
#include <stdint.h>

/* 初始化用于接收 MS901M 角度数据的 UART 中断。 */
void MS901M_Init(void);

/* 向帧解析器塞入 1 个 UART 字节，通常由中断函数调用。 */
void MS901M_PushByte(uint8_t byte);

/* 已经解析到至少一帧有效 yaw 数据时返回 true。 */
bool MS901M_Available(void);

/* 当前 yaw，单位为 0.01 度：4500 表示 45.00 度。 */
int16_t MS901M_GetYawCdeg(void);

/* 当前 roll，单位为 0.01 度。 */
int16_t MS901M_GetRollCdeg(void);

/* 当前 pitch，单位为 0.01 度。 */
int16_t MS901M_GetPitchCdeg(void);

/* 把当前 yaw 作为 0 度，供后续相对转向使用。 */
void MS901M_SetYawZero(void);

/* 计算从当前角到目标角的最短有符号误差，单位 0.01 度。 */
int16_t MS901M_YawErrorCdeg(int16_t target, int16_t current);

#endif
