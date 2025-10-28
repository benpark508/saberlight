// Lab7.c
// Runs on TM4C123
// Spring 2025

/*
// *******Inputs from switches**************************
// bit1 PE1 Voice switch, positive logic switch, external pull down
// bit0 PE0 Play switch, positive logic switch, external pull down

// ********Outputs to DAC****************************
// TLV5616 SSIClk (SCLK) connected to PD0
// TLV5616 SSIFss (FS)   connected to PD1
// TLV5616 SSITx (DIN)   connected to PD3

// *****LCD graphics, and I/O with SDC***************
// Backlight (pin 10) connected to +3.3 V
// MISO (pin 9) connected to PA4 (SSIRx for SDC)
// SCK (pin 8) connected to PA2 (SSI0Clk)
// MOSI (pin 7) connected to PA5 (SSI0Tx)
// TFT_CS (pin 6) connected to PA3 (SSI0Fss)
// CARD_CS (pin 5) connected to PD7 (CS for SDC)
// Data/Command (pin 4) connected to PA6 (GPIO)
// RESET (pin 3) connected to PA7 (GPIO)
// VCC (pin 2) connected to +3.3 V
// Gnd (pin 1) connected to ground

// Debugging, oscilloscope connected to
// PF1 toggles in background
// PF3 toggles in foreground
*/

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "music.h"
#include "../inc/PLL.h"
#include "../inc/SysTickInts.h"
#include "../inc/TLV5616.h"
#include "mailbox.h"
#include "../inc/BetterSwitch.h"
#include "../inc/GPIO_HAL.h"
#include "../inc/Unified_Port_Init.h"
#include "../inc/Timer0A.h"

#define DEBOUNCE_MS 20

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void WaitForInterrupt(void);  // low power mode

extern volatile bool PE0_flag;
extern volatile bool PE1_flag;
extern volatile bool PF4_flag;

extern volatile uint32_t PE0_start;
extern volatile uint32_t PE1_start;
extern volatile uint32_t PF4_start;

volatile bool PE0_read_pending = false;
volatile bool PE1_read_pending = false;
volatile bool PF4_read_pending = false;

bool PE0_pressed = false;
bool PE1_pressed = false;
bool PF4_pressed = false;
bool play_flag = false;
bool rewind_flag = false;
volatile uint32_t ticks_ms = 0;
volatile uint32_t b_count = 0;

void switch_task()
{
  ticks_ms++;
  if (PE0_flag && (ticks_ms - PE0_start) >= DEBOUNCE_MS)
  {
    PE0_flag = false;
    PE0_read_pending = true;
    GPIO_EnableInterrupt(GPIO_PORTE_BASE, 0);
  }
  if (PE1_flag && (ticks_ms - PE1_start) >= DEBOUNCE_MS)
  {
    PE1_flag = false;
    PE1_read_pending = true;
    GPIO_EnableInterrupt(GPIO_PORTE_BASE, 1);
  }
    if (PF4_flag && (ticks_ms - PF4_start) >= DEBOUNCE_MS)
  {
    PF4_flag = false;
    PF4_read_pending = true;
    GPIO_EnableInterrupt(GPIO_PORTF_BASE, 4);
  }
}

GPIO_PORT_ISR(4, GPIOPortE_Handler)
GPIO_PORT_ISR(5, GPIOPortF_Handler)

//----- Prototypes of functions in startup.s  ----------------------
//
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical(void);     // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // Go into low power mode

int main(void)
{
  DisableInterrupts();
  PLL_Init(Bus80MHz); // bus clock at 80 MHz
  // write this
  BetterSwitch_Init();
  Music_Init();
  Timer0A_Init(switch_task, 80000, 3); // 1 ms
  EnableInterrupts();

  while (1)
  {
    // write this
    if (PE0_read_pending)
    {
      PE0_read_pending = false;
      PE0_pressed = (GPIO_Read(GPIO_PORTE_BASE, 0) == 1);
    }
    if (PE1_read_pending)
    {
      PE1_read_pending = false;
      PE1_pressed = (GPIO_Read(GPIO_PORTE_BASE, 1) == 1);
    }
    if (PF4_read_pending)
    {
      PF4_read_pending = false;
      PF4_pressed = (GPIO_Read(GPIO_PORTF_BASE, 4) == 0);
    }
    if (PE0_pressed)
    {
      b_count++;
      PE0_pressed = false;
      play_flag = !play_flag;
      if (play_flag)
      {
        Play();
      }
      else
      {
        Stop();
      }
    }
    if (PE1_pressed)
    {
      b_count++;
      PE1_pressed = false;
      play_flag = false;
      Rewind();
    }
    if (PF4_pressed)
    {
      b_count++;
      PF4_pressed = false;
      ChangeTempo();
    }
  }
}
