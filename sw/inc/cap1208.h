// cap1208.h
// Runs on TM4C123
// Header file for cap1208.c
// Ben Park
// October 28 2025

#ifndef __CAP1208_H__
#define __CAP1208_H__

#include <stdint.h>
#include "../inc/I2C3.h"
#include "../inc/tm4c123gh6pm.h"

// DEVICE ADDRESS
#define CAP1208_ADDRESS              0x28

// REGISTER MAP
#define R_MAIN_CONTROL      0x00
#define R_GENERAL_STATUS    0x02
#define R_INPUT_STATUS      0x03
#define R_LED_STATUS        0x04
#define R_NOISE_FLAG_STATUS 0x0A

// Read-only delta counts
#define R_INPUT_1_DELTA     0x10
#define R_INPUT_2_DELTA     0x11
#define R_INPUT_3_DELTA     0x12
#define R_INPUT_4_DELTA     0x13
#define R_INPUT_5_DELTA     0x14
#define R_INPUT_6_DELTA     0x15
#define R_INPUT_7_DELTA     0x16
#define R_INPUT_8_DELTA     0x17

#define R_SENSITIVITY       0x1F
// B7     = N/A
// B6..B4 = Sensitivity
// B3..B0 = Base Shift

#define R_GENERAL_CONFIG    0x20
// B7 = Timeout
// B6 = Wake Config
// B5 = Disable Digital Noise
// B4 = Disable Analog Noise
// B3 = Max Duration Recalibration

#define R_INPUT_ENABLE      0x21
#define R_INPUT_CONFIG      0x22
#define R_INPUT_CONFIG2     0x23   // Default 0x07

#define R_SAMPLING_CONFIG   0x24   // Default 0x39
#define R_CALIBRATION       0x26   // Default 0x00
#define R_INTERRUPT_EN      0x27   // Default 0xFF
#define R_REPEAT_EN         0x28   // Default 0xFF
#define R_MTOUCH_CONFIG     0x2A   // Default 0xFF
#define R_MTOUCH_PAT_CONF   0x2B
#define R_MTOUCH_PATTERN    0x2D
#define R_COUNT_O_LIMIT     0x2E
#define R_RECALIBRATION     0x2F

// Per-input touch thresholds
#define R_INPUT_1_THRESH    0x30
#define R_INPUT_2_THRESH    0x31
#define R_INPUT_3_THRESH    0x32
#define R_INPUT_4_THRESH    0x33
#define R_INPUT_5_THRESH    0x34
#define R_INPUT_6_THRESH    0x35
#define R_INPUT_7_THRESH    0x36
#define R_INPUT_8_THRESH    0x37

// Noise threshold
#define R_NOISE_THRESH      0x38

// Standby Registers
#define R_STANDBY_CHANNEL   0x40
#define R_STANDBY_CONFIG    0x41
#define R_STANDBY_SENS      0x42
#define R_STANDBY_THRESH    0x43

#define R_CONFIGURATION     0x20
#define R_CONFIGURATION2    0x44
// B7 = Linked LED Transition
// B6 = Alert Polarity
// B5 = Reduce Power
// B4 = Link Polarity/Mirror
// B3 = Show RF Noise
// B2 = Disable RF Noise

// Read-only baseline counts
#define R_INPUT_1_BCOUNT    0x50
#define R_INPUT_2_BCOUNT    0x51
#define R_INPUT_3_BCOUNT    0x52
#define R_INPUT_4_BCOUNT    0x53
#define R_INPUT_5_BCOUNT    0x54
#define R_INPUT_6_BCOUNT    0x55
#define R_INPUT_7_BCOUNT    0x56
#define R_INPUT_8_BCOUNT    0x57


// Function prototypes
void CAP1208_Init(void);

void CAP1208_ReadInputs(uint8_t *result);

void CAP1208_GetInputs(uint8_t *result);

void CAP1208_ReadCount(uint8_t channel, int8_t *count);

void CAP1208_ClearINT(void);

#endif // __CAP1208_H__
