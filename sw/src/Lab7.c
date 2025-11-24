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
#include "../inc/cap1208.h"
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
volatile uint8_t touch;

int main(void)
{
  DisableInterrupts();
  Unified_Port_Init();
  PLL_Init(Bus80MHz); // bus clock at 80 MHz
  SysTick_Init();
  CAP1208_Init();
  ST7735_InitR_PortD(INITR_BLACKTAB_PortD); // initialize LCD
  EnableInterrupts();

  while (1)
  {
    touch = CAP1208_GetInputs();
    ST7735_FillRect_PortD(0, 0, ST7735_TFTWIDTH, 64, ST7735_BLACK); //clear text area
    ST7735_SetCursor_PortD(0, 0);
    ST7735_OutUDec_PortD(touch);
    ST7735_OutString_PortD("CAP1208 Demo\n");
    SysTick_Wait(100); // 1 ms 
  }
}
