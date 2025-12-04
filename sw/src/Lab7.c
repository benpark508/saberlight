// Lab7.c
// Runs on TM4C123
// Fall 2025

#include <stdint.h>
#include "mailbox.h"
#include "../inc/diskio.h"
#include "../inc/ff.h"
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
#include "../inc/Timer2A.h"
#include "ESP8266.h"
#include "UART4.h"
#include "lightstrip.h"
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
volatile uint8_t cap_read = 0;
volatile uint8_t imu_read = 0;
uint8_t touch = 0;
int8_t counts[2];
extern volatile uint8_t go;
static uint8_t last_any_touch = 0;

void READ_CAP(void)
{
  cap_read = 1;
}

void READ_IMU(void)
{
  imu_read = 1;
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
  long sr = StartCritical();
  MPU6500_calibrate(&imu_proc);
  EndCritical(sr);
  ST7735_InitR(INITR_GREENTAB);      // initialize LCD
  Timer1A_Init(READ_CAP, 800000, 4); // call every 10 ms
  Timer2A_Init(READ_IMU, 80000, 1);  // call every 1 ms
  Music_Init();
  Sound_ImperialMarch();
  Lightstrip_Init();
  ESP8266_Init(); // ESP stuff
  UART4_Init();
  ESP8266_Reset();
  EnableInterrupts();

  Blade_R = 255;
  Blade_G = 0;
  Blade_B = 0;
  char c;

  ST7735_FillScreen(ST7735_BLACK);
  ST7735_SetCursor(0, 0);
  ST7735_OutString("cap1208 demo");

  ST7735_SetCursor(0, 2); // Line 2
  ST7735_OutString("c1: ");
  ST7735_OutString("      ");

  ST7735_SetCursor(0, 4); // Line 2
  ST7735_OutString("c7: ");
  ST7735_OutString("      ");

  while (1)
  {
    if (imu_read)
    {
      imu_read = 0;
      MPU6500_getData(&imu_raw, &imu_proc);

      if (MPU6500_DetectSwing(&imu_proc) && !Music_IsPlaying())
      {
        Sound_Swing();
      }
    }

    if (cap_read)
    {
      cap_read = 0;

      CAP1208_ReadCounts(counts);

      ST7735_SetCursor(7, 2);
      ST7735_OutSDec8(counts[0]);
      ST7735_OutString("        ");
      ST7735_SetCursor(7, 4);
      ST7735_OutSDec8(counts[1]);
      ST7735_OutString("        ");

      uint8_t current_any_touch = 0;
      if (counts[0] == 127 || counts[1] == 127)
      {
          current_any_touch = 1;
      }

      // Edge Detection: Only trigger if we weren't touching before (Rising Edge)
      if (current_any_touch && !last_any_touch && !Music_IsPlaying())
      {
        Sound_Block();
      }
      
      // Update history
      last_any_touch = current_any_touch;
    }
  }
}
