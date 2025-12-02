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
#include "../inc/mpu6500.h"
#include "../inc/MCP4821.h"
#include "../inc/cap1208.h"
#include "../inc/SPI.h"
#include "../inc/ST7735.h"
#include "../inc/Timer1A.h"
#include "music.h"

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
uint8_t touch = 0;
int8_t count = 0;
extern volatile uint8_t go;

void debug_serial(void)
{
  printflag = 1;
}

int main(void)
{
  DisableInterrupts();
  Unified_Port_Init();
  PLL_Init(Bus80MHz); // bus clock at 80 MHz
  SysTick_Init();
  SPI_Init(200); // initialize SSI0 at 400 kHz
  CAP1208_Init();
  MPU6500_Init();
  FCLK_LCD();
  ST7735_InitR(INITR_GREENTAB);           // initialize LCD
  Timer1A_Init(debug_serial, 8000000, 2); // print every 100 ms
  Music_Init();
  EnableInterrupts();
  ST7735_FillScreen(ST7735_BLACK);
  ST7735_SetCursor(0, 0);
  ST7735_OutString("cap1208 demo");

  ST7735_SetCursor(0, 2); // Line 2
  ST7735_OutString("Delta: ");

  ST7735_SetCursor(0, 4); // Line 4
  ST7735_OutString("Touch: ");

  ST7735_SetCursor(0, 6); // Line 6
  ST7735_OutString("Pad 1: ");

  Music_Play();

  while (1)
  {
    if (printflag)
    {
      printflag = 0;

      CAP1208_ReadCount(1, &count);

      ST7735_SetCursor(7, 2);
      ST7735_OutSDec8(count);
      ST7735_OutString("   ");

      ST7735_SetCursor(7, 4);
      ST7735_OutSDec8(touch);
      ST7735_OutString("  ");

      ST7735_SetCursor(7, 6);
      if (touch & 0x01)
      {
        ST7735_SetTextColor(ST7735_GREEN);
        ST7735_OutString("YES");
      }
      else
      {
        ST7735_SetTextColor(ST7735_RED);
        ST7735_OutString("NO ");

        ST7735_SetTextColor(ST7735_YELLOW);
      }

      if (go)
      {
        go = 0;
        CAP1208_ReadInputs(&touch);
        CAP1208_ClearINT();
      }
    }
  }
}
