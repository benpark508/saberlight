// mpr121.h
// Runs on TM4C123
// Header file for mpr121.c
// Ben Park
// October 28 2025

#ifndef __MPR121_H__
#define __MPR121_H__

#include <stdint.h>
#include "../inc/I2C1.h"
#include "../inc/tm4c123gh6pm.h"

#define MPR121_ADDRESS              0x5A
#define MPR121_TOUCH_STATUS_L       0x00
#define MPR121_TOUCH_STATUS_H       0x01
#define MPR121_ELECTRODE_CONFIG     0x5E
#define MPR121_SOFT_RESET           0x80
#define MPR121_TOUCH_THRESHOLD_BASE 0x41
#define MPR121_RELEASE_THRESHOLD_BASE 0x42
#define MPR121_CONFIG1              0x5C
#define MPR121_CONFIG2              0x5D
#define MPR121_DEBOUNCE             0x5B

// Function prototypes
void MPR121_Init(void);
uint16_t MPR121_ReadTouchStatus(void);
int MPR121_SetElectrodeConfig(uint8_t config);
int MPR121_SetThresholds(uint8_t touchThreshold, uint8_t releaseThreshold);

#endif // __MPR121_H__
