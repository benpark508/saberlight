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
#include "../inc/cap1208.h"
#include "../inc/ST7735.h"
#include "../inc/Timer0A.h"

#define DAC_CS (*((volatile uint32_t *)0x40004008)) // PA1 bit-band
#define DAC_CS_LOW 0x00
#define DAC_CS_HIGH 0x02

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

GPIOConfig_t PB1Config = {
    .portBase = GPIO_PORTB_BASE,
    .pin = 1,
    .direction = GPIO_DIR_OUTPUT,
    .pull = false,
    .openDrain = false,
    .analog = false,
    .altFunc = false,
    .altFuncNum = 0,
    .intMode = GPIO_INT_DISABLE};

void debug_serial(void)
{
  touch = CAP1208_GetInputs();
  printflag = 1;
}

int main(void)
{
  DisableInterrupts();
  Unified_Port_Init();
  PLL_Init(Bus80MHz);       // bus clock at 80 MHz
  GPIO_Init(&PB1Config);
  GPIO_PORTA_DIR_R |= 0x02; // PA1 output
  GPIO_PORTA_DEN_R |= 0x02; // PA1 digital enable
  DAC_CS = DAC_CS_HIGH;     // deselect dac
  SysTick_Init();
  CAP1208_Init();
  MPU6500_Init();                         // initialize MPU6500, deselect cs pin
  ST7735_InitR(INITR_GREENTAB);           // initialize LCD
  Timer0A_Init(debug_serial, 8000000, 2); // print every 100 ms
  EnableInterrupts();

  while (1)
  {
    if (printflag)
    {
      printflag = 0;
      /*
      ST7735_FillRect(0, 0, ST7735_TFTWIDTH, 64, ST7735_BLACK); // clear text area
      ST7735_SetCursor(0, 0);
      ST7735_OutString("cap1208 demo\n");
      ST7735_OutString("val: ");
      ST7735_OutUDec(touch);
      */
      if(touch & 0x01){
        GPIO_Write(GPIO_PORTB_BASE, 1, 0);
      } else {
        GPIO_Write(GPIO_PORTB_BASE, 1, 1);
      }
    }
  }
}
