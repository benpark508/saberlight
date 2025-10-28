// TLV5616.c
// Runs on TM4C123
// Use SSI1 to send a 16-bit code to the TLV5616 and return the reply.
// Daniel Valvano
// EE445L Fall 2015
//    Jonathan W. Valvano 9/22/15

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2014

 Copyright 2014 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

// SSIClk (SCLK) connected to PD0
// SSIFss (FS)   connected to PD1
// SSITx (DIN)   connected to PD3

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/TLV5616.h"

//----------------   DAC_Init     -------------------------------------------
// Initialize TLV5616 12-bit DAC
// assumes bus clock is 80 MHz
// inputs: initial voltage output (0 to 4095)
// outputs:none
void DAC_Init(uint16_t data)
{
  // write this
  // Consider the following registers:
  // SYSCTL_RCGCSSI_R, SSI1_CR1_R, SSI1_CPSR_R, SSI1_CR0_R, SSI1_DR_R, SSI1_CR1_R

  SYSCTL_RCGCSSI_R |= 0x02;  // activate SSI1
  SYSCTL_RCGCGPIO_R |= 0x08; // activate port D
  while ((SYSCTL_PRGPIO_R & 0x08) == 0)
  {
  }; // allow time for clock to start
  GPIO_PORTD_LOCK_R = 0x4C4F434B; // 2) unlock GPIO Port D

  // initialize SSI1
  GPIO_PORTD_AFSEL_R |= 0x0B; // enable alt funct on PD0,1,3 (PA2,3,5)
  GPIO_PORTD_DEN_R |= 0x0B;   // enable digital I/O on PD0,1,3 (PA2,3,5)
                              // configure PD0,1,3 (PA2,3,5) as SSI
  GPIO_PORTD_PCTL_R = (GPIO_PORTD_PCTL_R & 0xFFFF0F00) + 0x00002022;
  GPIO_PORTD_AMSEL_R &= ~0x0B; // disable analog functionality on PD0,1,3 (PA2,3,5)
  SSI1_CR1_R &= ~SSI_CR1_SSE;  // disable SSI
  SSI1_CR1_R &= ~SSI_CR1_MS;   // master mode
                               // configure for system clock/PLL baud clock source
  SSI1_CC_R = (SSI1_CC_R & ~SSI_CC_CS_M) + SSI_CC_CS_SYSPLL;
  // Correct clock speed: 80MHz / (8 * (1 + 0)) = 10 MHz
  SSI1_CPSR_R = (SSI1_CPSR_R & ~SSI_CPSR_CPSDVSR_M) + 8;

  // Configure CR0 in a single, non-destructive operation
  SSI1_CR0_R = (SSI1_CR0_R & ~(SSI_CR0_SCR_M | SSI_CR0_SPO | SSI_CR0_FRF_M | SSI_CR0_DSS_M)) // 1. Clear all relevant fields
               | SSI_CR0_SPH                                                                 // 2. Set SPH = 1 (for Mode 1)
               | SSI_CR0_FRF_MOTO                                                            // 3. Set Freescale SPI format
               | SSI_CR0_DSS_16;                                                             // 4. Set 16-bit data size
  SSI1_CR1_R |= SSI_CR1_SSE;                                                                 // enable SSI
}

// --------------     DAC_Out   --------------------------------------------
// Send data to TLV5616 12-bit DAC
// inputs:  voltage output (0 to 4095)
//
void DAC_Out(uint16_t code)
{
  // write this
  // Consider the following registers:
  // SSI1_SR_R, SSI1_DR_R
  while ((SSI1_SR_R & SSI_SR_TNF) == 0)
  {
  }; // wait until transmit FIFO not full
  SSI1_DR_R = code;
}

// --------------     DAC_OutNonBlocking   ------------------------------------
// Send data to TLV5616 12-bit DAC without checking for room in the FIFO
// inputs:  voltage output (0 to 4095)
//
void DAC_Out_NB(uint16_t code)
{
  // Consider writing this (If it is what your heart desires)
  // Consider the following registers:
  // SSI1_SR_R, SSI1_DR_R
  SSI1_DR_R = code;
}
