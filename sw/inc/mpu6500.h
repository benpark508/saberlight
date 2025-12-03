// mpu6500.h
// Runs on TM4C123
// header file for mpu6500.c
// Ben Park
// October 28 2025

#ifndef __MPU6500_H__
#define __MPU6500_H__

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"

#define MPU6500_ADDRESS     0x68
#define WHO_AM_I           0x75
#define PWR_MGMT_1         0x6B
#define SMPLRT_DIV         0x19
#define CONFIG             0x1A
#define GYRO_CONFIG        0x1B
#define ACCEL_CONFIG       0x1C
#define ACCEL_XOUT_H       0x3B
#define ACCEL_XOUT_L       0x3C
#define ACCEL_YOUT_H       0x3D
#define ACCEL_YOUT_L       0x3E
#define ACCEL_ZOUT_H       0x3F
#define ACCEL_ZOUT_L       0x40
#define GYRO_XOUT_H       0x43
#define GYRO_XOUT_L       0x44
#define GYRO_YOUT_H       0x45
#define GYRO_YOUT_L       0x46
#define GYRO_ZOUT_H       0x47
#define GYRO_ZOUT_L       0x48
#define SIGNAL_PATH_RESET 0x68
#define USER_CTRL 0x6A

typedef struct {
    volatile int16_t accel_x;
    volatile int16_t accel_y;
    volatile int16_t accel_z;
    volatile int16_t gyro_x;
    volatile int16_t gyro_y;
    volatile int16_t gyro_z;
} raw_imu;

// Struct for processed IMU data
typedef struct {
	int16_t gyro_offX;
	int16_t gyro_offY;
	int16_t gyro_offZ;
    volatile int16_t accel_x;
    volatile int16_t accel_y;
    volatile int16_t accel_z;
    volatile int16_t gyro_x;
    volatile int16_t gyro_y;
    volatile int16_t gyro_z;
} processed_imu;



void MPU6500_Init(void);
void MPU6500_read_accel(raw_imu *imu_raw_data);
void MPU6500_read_gyro(raw_imu *imu_raw_data);
void MPU6500_getData(raw_imu *rData, processed_imu *pData);
void MPU6500_calibrate(uint16_t numCalPoints, processed_imu *imu_processed_data);

#endif // __MPU6500_H__