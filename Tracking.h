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

/* Above this normalized value, one sensor is treated as seeing the line. */
#define TRACKING_LINE_THRESHOLD  30U

/* Base motor speed and PD coefficients used by line-follow control. */
#define TRACKING_BASE_SPEED      2500
#define TRACKING_KP_NUM          48
#define TRACKING_KP_DEN          100
#define TRACKING_KD_NUM          2
#define TRACKING_KD_DEN          10

/* Latest normalized sensor readings, 0..TRACKING_VALUE_MAX. */
extern unsigned char LQ_Tracking_Value[TRACKING_CHANNEL_COUNT];

/* Weighted line position error: negative left, positive right. */
extern volatile int16_t Tracking_Error;

/* Latest PD correction added/subtracted from base wheel speed. */
extern volatile int16_t Tracking_Correction;

/* Nonzero when at least one channel is above the line threshold. */
extern volatile uint8_t Tracking_LineDetected;

/* Initialize ADC and mux-select GPIO state. */
void Tracking_Init(void);
void Tracking_Adc_Init(void);

/* Select one sensor channel by writing S2/S1/S0. */
void Tracking_IO_Set(unsigned char s2, unsigned char s1, unsigned char s0);

/* Read the currently selected sensor channel once. */
uint16_t Tracking_Adc_once(void);

/* Select and read one 1-based channel with simple averaging. */
unsigned int Tracking_Value_once(unsigned char ch);

/* Refresh all 8 sensor values. */
void Tracking_Value_Acquire(void);

/* Compute line-detected flag and weighted tracking error. */
int16_t Tracking_CalcError(void);

/* Standalone line-follow helper, not used by the current NAV state machine. */
void Tracking_LineFollowStep(void);

#endif
