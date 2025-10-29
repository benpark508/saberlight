// mpu9250.c
// Runs on TM4C123
// driver for mpu9250 9-axis IMU
// Ben Park
// October 28 2025

#include <stdint.h>
#include "../inc/mpu9250.h"
#include "../inc/SysTick.h"

void MPU9250_Init(void){

    uint8_t data[2];

    // Wake up the MPU9250
    data[0] = PWR_MGMT_1;
    data[1] = 0x01; // Set clock source to PLL with X axis gyroscope reference
    I2C1_Send(MPU9250_ADDRESS, data, 2);

    // check WHO_AM_I register
    data[0] = WHO_AM_I_MPU9250;
    I2C1_Send(MPU9250_ADDRESS, data, 1); // Set register address
    I2C1_Recv(MPU9250_ADDRESS, data, 1); // Read WHO_AM_I register
    if(data[0] != 0x71){
        // Handle error: device not found
        while(1);
    }

    // enable digital low pass filter
    data[0] = CONFIG;
    data[1] = 0x03; // DLPF_CFG = 3
    I2C1_Send(MPU9250_ADDRESS, data, 2);

    // Set accelerometer configuration
    data[0] = ACCEL_CONFIG;
    data[1] = 0x10; // ±8g
    I2C1_Send(MPU9250_ADDRESS, data, 2);

    // Set gyroscope configuration
    data[0] = GYRO_CONFIG;
    data[1] = 0x08; // ±500°/s
    I2C1_Send(MPU9250_ADDRESS, data, 2);

    // add additional configuration if needed
}

void MPU9250_read_accel(raw_imu *imu_raw_data){
    uint8_t data[6];
    data[0] = ACCEL_XOUT_H;
    I2C1_Send(MPU9250_ADDRESS, data, 1); // Set register address
    I2C1_Recv(MPU9250_ADDRESS, data, 6); // Read 6 bytes of accelerometer data

    imu_raw_data->accel_x = (data[0] << 8) | data[1]; // X-axis
    imu_raw_data->accel_y = (data[2] << 8) | data[3]; // Y-axis
    imu_raw_data->accel_z = (data[4] << 8) | data[5]; // Z-axis
}

void MPU9250_read_gyro(raw_imu *imu_raw_data){
    uint8_t data[6];
    data[0] = GYRO_XOUT_H;
    I2C1_Send(MPU9250_ADDRESS, data, 1); // Set register address
    I2C1_Recv(MPU9250_ADDRESS, data, 6); // Read 6 bytes of gyroscope data

    imu_raw_data->gyro_x = (data[0] << 8) | data[1]; // X-axis
    imu_raw_data->gyro_y = (data[2] << 8) | data[3]; // Y-axis
    imu_raw_data->gyro_z = (data[4] << 8) | data[5]; // Z-axis
}

void MPU9250_calibrate(uint16_t numCalPoints, processed_imu *imu_processed_data)
{
    // Init
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;

    // Zero guard
    if (numCalPoints == 0)
    {
        numCalPoints = 1;
    }

    raw_imu calib_data;

    // Save specified number of points
    for (uint16_t ii = 0; ii < numCalPoints; ii++)
    {
        MPU9250_read_gyro(&calib_data);
        x += calib_data.gyro_x;
        y += calib_data.gyro_y;
        z += calib_data.gyro_z;
        SysTick80_Wait10ms(1);
    }

    // Average the saved data points to find the gyroscope offset
    imu_processed_data->gyro_offX = x/numCalPoints;
    imu_processed_data->gyro_offY = y/numCalPoints;
    imu_processed_data->gyro_offZ = z/numCalPoints;
}

void MPU9250_getData(raw_imu *rData, processed_imu *pData){

    MPU9250_read_accel(rData);
    MPU9250_read_gyro(rData);

    // Process accelerometer data (if any processing is needed)
    pData->accel_x = rData->accel_x;
    pData->accel_y = rData->accel_y;
    pData->accel_z = rData->accel_z;

    // Process gyroscope data by removing offsets
    pData->gyro_x = rData->gyro_x - pData->gyro_offX;
    pData->gyro_y = rData->gyro_y - pData->gyro_offY;
    pData->gyro_z = rData->gyro_z - pData->gyro_offZ;
}