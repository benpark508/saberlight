// mpu9250.c
// Runs on TM4C123
// driver for mpu9250 9-axis IMU
// Ben Park
// October 28 2025

#include <stdint.h>
#include "../inc/mpu9250.h"
#include "../inc/SysTick.h"

uint8_t track = 0;

/*
void MPU9250_Init(void)
{
    // write this
    // Consider the following registers:
    // SYSCTL_RCGCSSI_R, SSI1_CR1_R, SSI1_CPSR_R, SSI1_CR0_R, SSI1_DR_R, SSI1_CR1_R

    SYSCTL_RCGCSSI_R |= 0x02;  // activate SSI1
    SYSCTL_RCGCGPIO_R |= 0x08; // activate port D
    while ((SYSCTL_PRGPIO_R & 0x08) == 0)
    {
    }; // allow time for clock to start
    GPIO_PORTD_LOCK_R = 0x4C4F434B; // 2) unlock GPIO Port D

    // initialize SSI1
    GPIO_PORTD_AFSEL_R |= 0x0B; // enable alt funct on PD0,1,3 (PA2,3,5)
    GPIO_PORTD_DEN_R |= 0x0B;   // enable digital I/O on PD0,1,3 (PA2,3,5)
                                // configure PD0,1,3 (PA2,3,5) as SSI
    GPIO_PORTD_PCTL_R = (GPIO_PORTD_PCTL_R & 0xFFFF0F00) + 0x00002022;
    GPIO_PORTD_AMSEL_R &= ~0x0B; // disable analog functionality on PD0,1,3 (PA2,3,5)
    SSI1_CR1_R &= ~SSI_CR1_SSE;  // disable SSI
    SSI1_CR1_R &= ~SSI_CR1_MS;   // master mode
                                 // configure for system clock/PLL baud clock source
    SSI1_CC_R = (SSI1_CC_R & ~SSI_CC_CS_M) + SSI_CC_CS_SYSPLL;
    // Correct clock speed: 80MHz / (8 * (1 + 0)) = 10 MHz
    SSI1_CPSR_R = (SSI1_CPSR_R & ~SSI_CPSR_CPSDVSR_M) + 8;

    // Configure CR0 in a single, non-destructive operation
    SSI1_CR0_R = (SSI1_CR0_R & ~(SSI_CR0_SCR_M | SSI_CR0_SPO | SSI_CR0_FRF_M | SSI_CR0_DSS_M)) // 1. Clear all relevant fields
                 | SSI_CR0_SPH                                                                 // 2. Set SPH = 1 (for Mode 1)
                 | SSI_CR0_FRF_MOTO                                                            // 3. Set Freescale SPI format
                 | SSI_CR0_DSS_16;                                                             // 4. Set 16-bit data size
    SSI1_CR1_R |= SSI_CR1_SSE;                                                                 // enable SSI
}

void MPU9250_read_accel(raw_imu *imu_raw_data)
{
    uint8_t data[6];
    data[0] = ACCEL_XOUT_H;
    track = I2C1_Send1(MPU9250_ADDRESS, data[0]); // Set register address
    track = I2C1_Recv(MPU9250_ADDRESS, data, 6);  // Read 6 bytes of accelerometer data

    imu_raw_data->accel_x = ((int16_t)data[0] << 8) | data[1]; // X-axis
    imu_raw_data->accel_y = (data[2] << 8) | data[3];          // Y-axis
    imu_raw_data->accel_z = (data[4] << 8) | data[5];          // Z-axis
}

void MPU9250_read_gyro(raw_imu *imu_raw_data)
{
    uint8_t data[6];
    data[0] = GYRO_XOUT_H;
    track = I2C1_Send1(MPU9250_ADDRESS, data[0]); // Set register address
    I2C1_Recv(MPU9250_ADDRESS, data, 6);          // Read 6 bytes of gyroscope data

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
    imu_processed_data->gyro_offX = x / numCalPoints;
    imu_processed_data->gyro_offY = y / numCalPoints;
    imu_processed_data->gyro_offZ = z / numCalPoints;
}

void MPU9250_getData(raw_imu *rData, processed_imu *pData)
{

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
    */