#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

/* 编码器速度值表示这个采样窗口内收到的脉冲数。 */
#define ENCODER_SAMPLE_MS 10U

/* 如果 OLED 上显示的速度方向反了，就把对应宏改成 -1。 */
#define ENCODER_LEFT_DIR  1
#define ENCODER_RIGHT_DIR 1

/* 最近一次采样得到的左右轮脉冲数，可用于 OLED 显示或速度反馈。 */
extern volatile int16_t Encoder_LeftSpeed;
extern volatile int16_t Encoder_RightSpeed;

/* 配置编码器 GPIO 中断状态。 */
void Encoder_Init(void);

/* 读取中断累计脉冲数，并把累计值清零。 */
void Encoder_Update(void);

/* 返回最近一次采样值。 */
int16_t Encoder_GetLeftSpeed(void);
int16_t Encoder_GetRightSpeed(void);

/* 如果知道每圈脉冲数，可以用这个函数把采样值换算成 RPM。 */
int32_t Encoder_CountToRPM(int16_t count, uint16_t sampleMs, uint16_t countsPerRev);

#endif
