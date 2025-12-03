// MCP4821_ManualCS.c
// Driver for MCP4821 using SSI0 on TM4C123
// Manual chip select on PA1

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/MCP4821.h"
#include "../inc/SPI.h"

long StartCritical(void);
void EndCritical(long sr);

//=========================================
// Initialize SSI0 + PA1 for MCP4821 DAC
//=========================================
void DAC_Init(uint16_t initial)
{
  uint16_t cmd = 0x3000 | (initial & 0x0FFF);
  select_DAC();
  SSI0_DR_R = cmd;
  while ((SSI0_SR_R & SSI_SR_TFE) == 0)
  {
  }; // wait empty
  deselect_DAC();
}

//=========================================
// Write to DAC (blocking)
//=========================================
void DAC_Out(uint16_t code)
{
  long sr = StartCritical();

  while ((SSI0_SR_R & SSI_SR_BSY) == SSI_SR_BSY)
  {
  };
  uint16_t command = 0x3000 | (code & 0x0FFF);

  // Wait for room in FIFO
  while ((SSI0_SR_R & SSI_SR_TNF) == 0)
  {
  }

  select_DAC();
  while ((SSI0_SR_R & SSI_SR_TNF) == 0)
  {
  };
  SSI0_DR_R = command >> 8;
  while ((SSI0_SR_R & SSI_SR_TNF) == 0)
  {
  };
  SSI0_DR_R = command & 0xFF;

  // Must wait for transmission to finish before releasing CS
  while ((SSI0_SR_R & SSI_SR_BSY) == SSI_SR_BSY)
  {
  };
  volatile uint32_t dummy;
  dummy = SSI0_DR_R;
  dummy = SSI0_DR_R;
  deselect_DAC();
  EndCritical(sr);
}

//=========================================
// Write to DAC (non-blocking)
//  (caller must ensure FIFO has room)
//=========================================
void DAC_Out_NB(uint16_t code)
{
  uint16_t command = 0x3000 | (code & 0x0FFF);

  select_DAC();
  SSI0_DR_R = command;
  // Do NOT release CS � caller must handle timing
}