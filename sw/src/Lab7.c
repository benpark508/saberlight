// Lab7.c
// Runs on TM4C123
// Fall 2025

#include <stdint.h>
#include "mailbox.h"
#include "../inc/tm4c123gh6pm.h"
#include "../inc/PLL.h"
#include "../inc/SysTick.h"
#include "../inc/GPIO_HAL.h"
#include "../inc/Unified_Port_Init.h"
#include "../inc/I2C1.h"
#include "../inc/mpu9250.h"
#include "../inc/mpr121.h"
#include "../inc/ST7735_PortD.h"

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void WaitForInterrupt(void);  // low power mode

//----- Prototypes of functions in startup.s  ----------------------
//
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical(void);     // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // Go into low power mode

raw_imu imu_raw;
processed_imu imu_proc;
volatile uint16_t touch;

int main(void)
{
  DisableInterrupts();
  PLL_Init(Bus80MHz);          // bus clock at 80 MHz
  SysTick_Init();
  I2C1_Init(400000, 80000000); // I2C at 400 Khz (fast-mode)
  MPU9250_Init();
  MPR121_Init();
  ST7735_InitR_PortD(INITR_BLACKTAB_PortD); // initialize LCD
  Unified_Port_Init();
  EnableInterrupts();

  MPU9250_calibrate(100, &imu_proc);

  while (1)
  {
    MPU9250_getData(&imu_raw, &imu_proc);
    touch = MPR121_ReadTouchStatus();

    ST7735_FillScreen_PortD(0); // clear screen
    ST7735_SetCursor_PortD(0,0); // set cursor to (0,0)
    ST7735_OutString_PortD("AX:");
    ST7735_OutUDec_PortD(imu_proc.accel_x);
    ST7735_OutString_PortD(" AY:");
    ST7735_OutUDec_PortD(imu_proc.accel_y);
    ST7735_OutString_PortD(" AZ:");
    ST7735_OutUDec_PortD(imu_proc.accel_z);
    ST7735_OutString_PortD("\nGX:");
    ST7735_OutUDec_PortD(imu_proc.gyro_x);
    ST7735_OutString_PortD(" GY:");
    ST7735_OutUDec_PortD(imu_proc.gyro_y);
    ST7735_OutString_PortD(" GZ:");
    ST7735_OutUDec_PortD(imu_proc.gyro_z);
    ST7735_OutString_PortD("\nTouch:");
    ST7735_OutUDec_PortD(touch);

    SysTick_Wait(80); // 1 ms
  }
}
