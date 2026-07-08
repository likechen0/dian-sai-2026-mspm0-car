#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

/* Encoder speed is reported as counts collected during this sample window. */
#define ENCODER_SAMPLE_MS 10U

/* Change to -1 if the displayed speed direction is opposite. */
#define ENCODER_LEFT_DIR  1
#define ENCODER_RIGHT_DIR 1

/* Latest sampled encoder counts for OLED display or speed feedback. */
extern volatile int16_t Encoder_LeftSpeed;
extern volatile int16_t Encoder_RightSpeed;

/* Configure encoder GPIO interrupt state. */
void Encoder_Init(void);

/* Snapshot interrupt counts and reset the accumulators. */
void Encoder_Update(void);

/* Return the latest sampled counts. */
int16_t Encoder_GetLeftSpeed(void);
int16_t Encoder_GetRightSpeed(void);

/* Optional helper when the encoder counts-per-revolution is known. */
int32_t Encoder_CountToRPM(int16_t count, uint16_t sampleMs, uint16_t countsPerRev);

#endif
