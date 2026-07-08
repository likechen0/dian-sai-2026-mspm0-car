#ifndef TRACKING_H
#define TRACKING_H

#include <stdint.h>

/*
 * LongQiu 8-channel grayscale module SC wiring:
 * AS -> PA25(ADC0_CH2), S0 -> PA24, S1 -> PA26, S2 -> PA27.
 */
#define TRACKING_VALUE_MAX       100U
#define TRACKING_CHANNEL_COUNT   8U

/* Set to 0 if black line gives a smaller ADC value than white floor. */
#define TRACKING_BLACK_IS_HIGH   1
#define TRACKING_LINE_THRESHOLD  30U

#define TRACKING_BASE_SPEED      2500
#define TRACKING_KP_NUM          48
#define TRACKING_KP_DEN          100
#define TRACKING_KD_NUM          2
#define TRACKING_KD_DEN          10

extern unsigned char LQ_Tracking_Value[TRACKING_CHANNEL_COUNT];
extern volatile int16_t Tracking_Error;
extern volatile int16_t Tracking_Correction;
extern volatile uint8_t Tracking_LineDetected;

void Tracking_Init(void);
void Tracking_Adc_Init(void);
void Tracking_IO_Set(unsigned char s2, unsigned char s1, unsigned char s0);
uint16_t Tracking_Adc_once(void);
unsigned int Tracking_Value_once(unsigned char ch);
void Tracking_Value_Acquire(void);
int16_t Tracking_CalcError(void);
void Tracking_LineFollowStep(void);

#endif
