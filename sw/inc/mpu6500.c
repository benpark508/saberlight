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

#define MPU_CS (*((volatile uint32_t *)0x40007040))
#define LCD_CS (*((volatile uint32_t *)0x40004100))
#define MPU_READ 0x80
#define MPU_WRITE 0x00

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
    volatile uint32_t delay;

    SYSCTL_RCGCGPIO_R |= 0x08;  // activate port D
    while ((SYSCTL_PRGPIO_R & 0x08) == 0)
    {
    };

    GPIO_PORTD_DIR_R |= 0x10; // PD4 Output
    GPIO_PORTD_DEN_R |= 0x10; // PD4 Digital Enable
    MPU_CS = 0x10;            // Set High (Deselect)

    // 3. Perform Reset & Configuration
    uint32_t lcd_speed = SSI0_CPSR_R; // Save LCD speed
    WaitForSSI0Idle();
    SSI0_CPSR_R = 80;

    MPU6500_WriteReg(PWR_MGMT_1, 0x80); 
    SysTick80_Wait10ms(50); // wait 500 ms

    MPU6500_WriteReg(PWR_MGMT_1, 0x00);

    MPU6500_WriteReg(CONFIG, 0x03); // DLPF_CFG = 3

    MPU6500_WriteReg(ACCEL_CONFIG, 0x10); // ±8g

    MPU6500_WriteReg(GYRO_CONFIG, 0x08); // ±500°/s

    WaitForSSI0Idle();
    SSI0_CPSR_R = lcd_speed; 
}

void MPU6500_WriteReg(uint8_t reg, uint8_t data){

    WaitForSSI0Idle();
    LCD_CS = 0x40; 
    MPU_CS = 0; 
    
    while((SSI0_SR_R & 0x02) == 0){}; // Wait for TX FIFO space
    SSI0_DR_R = reg | MPU_WRITE;
    
    while((SSI0_SR_R & 0x02) == 0){};
    SSI0_DR_R = data;
    
    WaitForSSI0Idle();
    
    uint8_t trash;
    while((SSI0_SR_R & 0x04) != 0) { // While RX FIFO not empty
        trash = SSI0_DR_R;
    }
    
    MPU_CS = 0x10;
}

// Read a register
uint8_t MPU6500_ReadReg(uint8_t reg){
    uint8_t trash, result;
    
    WaitForSSI0Idle();
    LCD_CS = 0x40;  // Force LCD Deselect
    MPU_CS = 0;     // Select MPU
    
    // 1. Send Register Address (Read)
    while((SSI0_SR_R & 0x02) == 0){};
    SSI0_DR_R = reg | MPU_READ;
    
    // 2. Send Dummy Byte (to generate clock for MPU to send data)
    while((SSI0_SR_R & 0x02) == 0){};
    SSI0_DR_R = 0x00;
    
    WaitForSSI0Idle();
    
    // 3. Read FIFO
    // We sent 2 bytes, we expect 2 bytes back.
    // Byte 1: Garbage (Response to Register Address)
    // Byte 2: Data (Response to Dummy Byte)
    
    while((SSI0_SR_R & 0x04) == 0){}; // Wait for RX Data
    trash = SSI0_DR_R; 
    
    while((SSI0_SR_R & 0x04) == 0){}; // Wait for RX Data
    result = SSI0_DR_R;
    
    MPU_CS = 0x10; // Deselect
    return result;
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