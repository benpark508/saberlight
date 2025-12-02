// mpu6500.c
// Runs on TM4C123
// driver for mpu6500 9-axis IMU
// Ben Park
// October 28 2025

#include <stdint.h>
#include "../inc/mpu6500.h"
#include "../inc/SysTick.h"
#include "../inc/tm4c123gh6pm.h"
#include "../inc/hw_types.h"
#include "../inc/SPI.h"

#define MPU_CS (*((volatile uint32_t *)0x40007040)) // PD4 bit-band
#define MPU_CS_LOW 0x00
#define MPU_CS_HIGH 0x10
#define MPU_READ 0x80
#define MPU_WRITE 0x00

long StartCritical(void);
void EndCritical(long sr);

void WaitForSSI0Idle(void)
{
    while ((SSI0_SR_R & 0x01) == 0)
    {
    }; // Wait for TX FIFO empty
    while ((SSI0_SR_R & 0x10) == 0x10)
    {
    }; // Wait for Busy bit low
}

void MPU6500_Init(void)
{
    long sr = StartCritical();

    while ((SSI0_SR_R & 0x10) == 0x10)
    {
    };

    FCLK_SLOW();

    select_IMU();

    uint8_t who_am_i = MPU6500_ReadReg(WHO_AM_I);
    if (who_am_i != 0x70)
    {
        // Communication error or wrong chip! Loop indefinitely or flag an error.
        while (1)
        {
            // Blink an LED here for visual debug
        }
    }

    xchg_spi(PWR_MGMT_1 | MPU_WRITE);
    xchg_spi(0x80); // Reset
    deselect_IMU();

    while ((SSI0_SR_R & 0x10) == 0x10)
    {
    };

    FCLK_LCD();

    EndCritical(sr);

    SysTick80_Wait10ms(50); // wait 500 ms

    sr = StartCritical();

    while ((SSI0_SR_R & 0x10) == 0x10)
    {
    };

    FCLK_SLOW();

    select_IMU();
    xchg_spi(PWR_MGMT_1 | MPU_WRITE);
    xchg_spi(0x00); // Wake Up
    deselect_IMU();

    select_IMU();
    xchg_spi(CONFIG | MPU_WRITE);
    xchg_spi(0x03); // DLPF
    deselect_IMU();

    select_IMU();
    xchg_spi(ACCEL_CONFIG | MPU_WRITE);
    xchg_spi(0x10); // 8g
    deselect_IMU();

    select_IMU();
    xchg_spi(GYRO_CONFIG | MPU_WRITE);
    xchg_spi(0x08); // 500dps
    deselect_IMU();

    while ((SSI0_SR_R & 0x10) == 0x10)
    {
    };

    FCLK_LCD();

    EndCritical(sr);
}

void MPU6500_WriteReg(uint8_t reg, uint8_t data)
{
    long sr = StartCritical();

    while ((SSI0_SR_R & 0x10) == 0x10)
    {
    };

    FCLK_SLOW();
    select_IMU();              // CS Low
    xchg_spi(reg | MPU_WRITE); // Send Register Address
    xchg_spi(data);            // Send Data
    deselect_IMU();            // CS High

    while ((SSI0_SR_R & 0x10) == 0x10)
    {
    };

    FCLK_LCD();
    EndCritical(sr);
}

// Read a register
uint8_t MPU6500_ReadReg(uint8_t reg)
{
    long sr = StartCritical();

    while ((SSI0_SR_R & 0x10) == 0x10)
    {
    };

    FCLK_SLOW();
    uint8_t result;
    select_IMU();             // CS Low
    xchg_spi(reg | MPU_READ); // Send Register Address (Read Bit Set)
    result = xchg_spi(0xFF);  // Send Dummy to clock in data
    deselect_IMU();           // CS High

    while ((SSI0_SR_R & 0x10) == 0x10)
    {
    };

    FCLK_LCD();
    EndCritical(sr);
    return result;
}

void MPU6500_ReadBlock(uint8_t reg, uint8_t *data, uint8_t length)
{
    long sr = StartCritical();

    while ((SSI0_SR_R & 0x10) == 0x10)
    {
    };

    select_IMU();             // CS Low
    xchg_spi(reg | MPU_READ); // Send Start Register Address

    for (int i = 0; i < length; i++)
    {
        data[i] = xchg_spi(0xFF); // Send Dummy to clock in data
    }

    deselect_IMU(); // CS High
    EndCritical(sr);
}

void MPU6500_calibrate(uint16_t numCalPoints, processed_imu *imu_processed_data)
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
        MPU6500_read_gyro(&calib_data);
        x += calib_data.gyro_x;
        y += calib_data.gyro_y;
        z += calib_data.gyro_z;
        SysTick80_Wait10ms(1);
    }

    // Average the saved data points to find the gyroscope offset
    imu_processed_data->gyro_offX = x / numCalPoints;
    imu_processed_data->gyro_offY = y / numCalPoints;
    imu_processed_data->gyro_offZ = z / numCalPoints;
}

void MPU6500_getData(raw_imu *rData, processed_imu *pData)
{

    MPU6500_read_accel(rData);
    MPU6500_read_gyro(rData);

    // Process accelerometer data (if any processing is needed)
    pData->accel_x = rData->accel_x;
    pData->accel_y = rData->accel_y;
    pData->accel_z = rData->accel_z;

    // Process gyroscope data by removing offsets
    pData->gyro_x = rData->gyro_x - pData->gyro_offX;
    pData->gyro_y = rData->gyro_y - pData->gyro_offY;
    pData->gyro_z = rData->gyro_z - pData->gyro_offZ;
}

void MPU6500_read_accel(raw_imu *imu_raw_data)
{
    uint8_t imu_data[6];
    MPU6500_ReadBlock(ACCEL_XOUT_H, imu_data, 6);

    imu_raw_data->accel_x = (int16_t)(((uint16_t)imu_data[0] << 8) | imu_data[1]);
    imu_raw_data->accel_y = (int16_t)(((uint16_t)imu_data[2] << 8) | imu_data[3]);
    imu_raw_data->accel_z = (int16_t)(((uint16_t)imu_data[4] << 8) | imu_data[5]);
}

void MPU6500_read_gyro(raw_imu *imu_raw_data)
{
    uint8_t imu_data[6];
    MPU6500_ReadBlock(GYRO_XOUT_H, imu_data, 6);

    imu_raw_data->gyro_x = (int16_t)(((uint16_t)imu_data[0] << 8) | imu_data[1]);
    imu_raw_data->gyro_y = (int16_t)(((uint16_t)imu_data[2] << 8) | imu_data[3]);
    imu_raw_data->gyro_z = (int16_t)(((uint16_t)imu_data[4] << 8) | imu_data[5]);
}