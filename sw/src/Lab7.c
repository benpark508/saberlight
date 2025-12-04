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
#include "../inc/Timer2A.h"
#include "ESP8266.h"
#include "UART4.h"
#include "game.h"
#include "lightstrip.h"
#include "music.h"
#include "microSD.h"

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

uint8_t flag = 0;
raw_imu imu_raw;
processed_imu imu_proc;
volatile uint8_t cap_read = 0;
volatile uint8_t imu_read = 0;
uint8_t touch = 0;
int8_t counts[2];
extern volatile uint8_t go;
static uint8_t last_any_touch = 0;

void run_game(void)
{
  flag = 1;
}

int main(void)
{
  DisableInterrupts();
  Unified_Port_Init();
  PLL_Init(Bus80MHz); // bus clock at 80 MHz
  SysTick_Init();

  SPI_Init(200); // initialize SSI0 at 400 kHz
  UART4_Init();
  Timer2A_Init(run_game, 80000, 2); // call every 1 ms

  CAP1208_Init();
  MPU6500_Init();

  ST7735_InitR(INITR_GREENTAB); // initialize LCD
  Music_Init();
  Lightstrip_Init();

  ESP8266_Init(); // ESP stuff
  ESP8266_Reset();

  ST7735_FillScreen(ST7735_BLACK);
  ST7735_SetCursor(0, 0);
  ST7735_OutString("Calibrating IMU...");

  long sr = StartCritical();
  MPU6500_calibrate(&imu_proc);
  EndCritical(sr);

  ST7735_OutString("Done.");
  SysTick_Wait10ms(50); // Short pause to read message

  Game_Init();

  // Set initial visual state
  Lightstrip_SetAnimation(ANIM_IDLE);

  // 8. Start Everything
  EnableInterrupts();

  SD_Init();

  FCLK_FAST();

  while (1)
  {
    if (flag)
    {
      flag = 0;
      Game_Run();
    }
  } 
}
