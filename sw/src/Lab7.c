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
#include "../inc/I2C3.h"
#include "../inc/mpu9250.h"
#include "../inc/cap1208.h"
#include "../inc/ST7735.h"
#include "../inc/Timer0A.h"

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
volatile uint8_t printflag = 0;
volatile uint8_t touch = 0;

void debug_serial(void)
{
  touch = CAP1208_GetInputs();
  printflag = 1;
}

int main(void)
{
  DisableInterrupts();
  Unified_Port_Init();
  PLL_Init(Bus80MHz); // bus clock at 80 MHz
  SysTick_Init();
  CAP1208_Init();
  ST7735_InitR(INITR_BLACKTAB);           // initialize LCD
  Timer0A_Init(debug_serial, 8000000, 2); // print every 100 ms
  EnableInterrupts();

  while (1)
  {
    if (printflag)
    {
      printflag = 0;
      ST7735_FillRect(0, 0, ST7735_TFTWIDTH, 64, ST7735_BLACK); // clear text area
      ST7735_SetCursor(0, 0);
      ST7735_OutString("cap1208 demo\n");
      ST7735_OutString("val: ");
      ST7735_OutUDec(touch);
    }
  }
}
