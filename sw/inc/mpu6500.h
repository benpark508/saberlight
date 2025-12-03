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

typedef struct {
    // Offsets
    int32_t gyro_off_x;
    int32_t gyro_off_y;
    int32_t gyro_off_z;
    int32_t accel_off_x;
    int32_t accel_off_y;
    int32_t accel_off_z;

    // Real values in milli-units (e.g. 1000 = 1.0)
    int32_t accel_x_mg;    // milli-g
    int32_t accel_y_mg;
    int32_t accel_z_mg;
    int32_t gyro_x_mdps;   // milli-degrees per second
    int32_t gyro_y_mdps;
    int32_t gyro_z_mdps;
} processed_imu;



void MPU6500_Init(void);
uint8_t MPU6500_ReadReg(uint8_t reg);
void MPU6500_read_accel(raw_imu *imu_raw_data);
void MPU6500_read_gyro(raw_imu *imu_raw_data);
void MPU6500_getData(raw_imu *rData, processed_imu *pData);
void MPU6500_calibrate(processed_imu *imu_processed_data);

#endif // __MPU6500_H__