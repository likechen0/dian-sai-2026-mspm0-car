#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

#define ENCODER_SAMPLE_MS 10U

/* Change to -1 if the displayed speed direction is opposite. */
#define ENCODER_LEFT_DIR  1
#define ENCODER_RIGHT_DIR 1

extern volatile int16_t Encoder_LeftSpeed;
extern volatile int16_t Encoder_RightSpeed;

void Encoder_Init(void);
void Encoder_Update(void);
int16_t Encoder_GetLeftSpeed(void);
int16_t Encoder_GetRightSpeed(void);
int32_t Encoder_CountToRPM(int16_t count, uint16_t sampleMs, uint16_t countsPerRev);

#endif
