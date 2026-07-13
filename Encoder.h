#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

/* 编码器速度值表示这个采样窗口内收到的脉冲数。 */
#define ENCODER_SAMPLE_MS 10U

/*
 * 逻辑左轮：U16 的 A2/B2；逻辑右轮：U6 的 A1/B1。
 * 如果正确前进时某一侧 OLED 显示负数，只修改对应 DIR，不改电机方向。
 */
#define ENCODER_LEFT_DIR  1
#define ENCODER_RIGHT_DIR 1

#define ENCODER_MM_PER_COUNT_NUM 1
#define ENCODER_MM_PER_COUNT_DEN 1

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
int32_t Encoder_GetLeftTotalCount(void);
int32_t Encoder_GetRightTotalCount(void);
int32_t Encoder_GetXmm(void);
int32_t Encoder_GetYmm(void);
int16_t Encoder_GetPoseYawCdeg(void);
void Encoder_ResetOdometry(void);

/* 如果知道每圈脉冲数，可以用这个函数把采样值换算成 RPM。 */
int32_t Encoder_CountToRPM(int16_t count, uint16_t sampleMs, uint16_t countsPerRev);

#endif
