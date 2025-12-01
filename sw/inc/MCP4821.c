// MCP4821_ManualCS.c
// Driver for MCP4821 using SSI0 on TM4C123
// Manual chip select on PA1

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/MCP4821.h"
#include "../inc/SPI.h"

// =======================
//  Helper: Delay 1 cycle
// =======================
static inline void CS_LOW(void){
  GPIO_PORTA_DATA_R &= ~0x02;   // PA1 = 0
}
static inline void CS_HIGH(void){
  GPIO_PORTA_DATA_R |= 0x02;    // PA1 = 1
}


//=========================================
// Initialize SSI0 + PA1 for MCP4821 DAC
//=========================================
void DAC_Init(uint16_t initial){
  uint16_t cmd = 0x3000 | (initial & 0x0FFF);
  select_DAC();
  SSI0_DR_R = cmd;
  while((SSI0_SR_R & SSI_SR_TFE) == 0){};   // wait empty
  deselect_DAC();
}


//=========================================
// Write to DAC (blocking)
//=========================================
void DAC_Out(uint16_t code){
  uint16_t command = 0x3000 | (code & 0x0FFF);

  // Wait for room in FIFO
  while((SSI0_SR_R & SSI_SR_TNF) == 0){}

  select_DAC();
  SSI0_DR_R = command;

  // Must wait for transmission to finish before releasing CS
  while((SSI0_SR_R & SSI_SR_BSY) != 0){}
  deselect_DAC();
}


//=========================================
// Write to DAC (non-blocking)
//  (caller must ensure FIFO has room)
//=========================================
void DAC_Out_NB(uint16_t code){
  uint16_t command = 0x3000 | (code & 0x0FFF);

  select_DAC();
  SSI0_DR_R = command;
  // Do NOT release CS � caller must handle timing
}