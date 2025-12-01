// SPI.c
// Runs on LM4F120/TM4C123
// uses SSI0

/* This example accompanies the books
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2017
   "Embedded Systems: Real-Time Operating Systems for ARM Cortex-M Microcontrollers",
   ISBN: 978-1466468863, Jonathan Valvano, copyright (c) 2017

 Copyright 2017 by Jonathan W. Valvano, valvano@mail.utexas.edu
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
// SDC CS as PA7

// hardware connections
// **********ST7735 TFT and SDC*******************
// ST7735
// Backlight (pin 10) connected to +3.3 V
// MISO (pin 9) connected to PA4 (SSI0Rx)
// SCK (pin 8) connected to PA2 (SSI0Clk)
// MOSI (pin 7) connected to PA5 (SSI0Tx)
// TFT_CS (pin 6) connected to PA6 (SSI0Fss)
// CARD_CS (pin 5) connected to PA7
// Data/Command (pin 4) connected to PF0 (GPIO), high for data, low for command
// RESET (pin 3) connected to PF1 (GPIO)
// VCC (pin 2) connected to +3.3 V
// Gnd (pin 1) connected to ground

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "SPI.h"

#define SSI_CR0_SCR_M 0x0000FF00      // SSI Serial Clock Rate
#define SSI_CR0_SPH 0x00000080        // SSI Serial Clock Phase
#define SSI_CR0_SPO 0x00000040        // SSI Serial Clock Polarity
#define SSI_CR0_FRF_M 0x00000030      // SSI Frame Format Select
#define SSI_CR0_FRF_MOTO 0x00000000   // Freescale SPI Frame Format
#define SSI_CR0_DSS_M 0x0000000F      // SSI Data Size Select
#define SSI_CR0_DSS_8 0x00000007      // 8-bit data
#define SSI_CR1_MS 0x00000004         // SSI Master/Slave Select
#define SSI_CR1_SSE 0x00000002        // SSI Synchronous Serial Port
                                      // Enable
#define SSI_SR_BSY 0x00000010         // SSI Busy Bit
#define SSI_SR_TNF 0x00000002         // SSI Transmit FIFO Not Full
#define SSI_CPSR_CPSDVSR_M 0x000000FF // SSI Clock Prescale Divisor
#define SSI_CC_CS_M 0x0000000F        // SSI Baud Clock Source
#define SSI_CC_CS_SYSPLL 0x00000000   // Either the system clock (if the
                                      // PLL bypass is in effect) or the
                                      // PLL output (default)
#define SYSCTL_RCGC1_SSI0 0x00000010  // SSI0 Clock Gating Control
#define SYSCTL_RCGC2_GPIOA 0x00000001 // port A Clock Gating Control

#define SDC_CS (*((volatile unsigned long *)0x40004200)) // PA7
#define SDC_CS_LOW 0
#define SDC_CS_HIGH 0x80

#define TFT_CS (*((volatile unsigned long *)0x40004100)) // PA6
#define TFT_CS_LOW 0
#define TFT_CS_HIGH 0x40

#define IMU_CS (*((volatile unsigned long *)0x40007040)) // PD4
#define IMU_CS_LOW 0
#define IMU_CS_HIGH 0x10

#define DAC_CS (*((volatile unsigned long *)0x40004008)) // PA1
#define DAC_CS_LOW 0
#define DAC_CS_HIGH 0x02

static volatile UINT Timer1, Timer2;

// initialize for SPI
// assumes: system clock rate is 80 MHz, SCR=0
// SSIClk = SysClk / (CPSDVSR * (1 + SCR)) = 80 MHz/CPSDVSR
// 200 for    400,000 bps slow mode, used during initialization
// 8   for 10,000,000 bps fast mode, used during disk I/O
void SPI_Init(unsigned long CPSDVSR)
{
    volatile uint32_t delay;
    SYSCTL_RCGCSSI_R |= 0x01;  // activate SSI0
    SYSCTL_RCGCGPIO_R |= 0x01; // activate port A
    while ((SYSCTL_PRGPIO_R & 0x01) == 0)
    {
    }; // allow time for clock to start

    // initialize TFT CS
    GPIO_PORTA_DIR_R |= 0x40;         // PA6 = CS
    GPIO_PORTA_AFSEL_R &= ~0x40;      // GPIO
    GPIO_PORTA_DEN_R |= 0x40;         // digital enable
    GPIO_PORTA_AMSEL_R &= ~0x40;      // no analog
    GPIO_PORTA_PCTL_R &= ~0x0F000000; // PA6 as GPIO
    TFT_CS = TFT_CS_HIGH;             // deselect TFT

    // initialize DAC CS
    GPIO_PORTA_DIR_R |= 0x02;         // PA1 output
    GPIO_PORTA_AFSEL_R &= ~0x02;      // GPIO
    GPIO_PORTA_DEN_R |= 0x02;         // PA1 digital enable
    GPIO_PORTA_AMSEL_R &= ~0x02;      // disable analog functionality on PA1
    GPIO_PORTA_PCTL_R &= ~0x000000F0; // PA1 as GPIO
    DAC_CS = DAC_CS_HIGH;             // deselect dac

    // initialize SDC CS
    GPIO_PORTA_PUR_R |= 0x80;  // enable weak pullup on PB0
    GPIO_PORTB_DIR_R |= 0x80;  // make PB0 output
    GPIO_PORTB_DR4R_R |= 0x80; // 4mA output on outputs
    SDC_CS = SDC_CS_HIGH;      // deselect SDC
    GPIO_PORTB_PCTL_R &= ~0xF0000000;
    GPIO_PORTB_AMSEL_R &= ~0x80; // disable analog functionality on PB0
    GPIO_PORTB_DEN_R |= 0x80;    // enable digital I/O on PB0

    // initialize SSI0
    GPIO_PORTA_AFSEL_R |= 0x34; // enable alt funct on PA2,4,5
    GPIO_PORTA_PUR_R |= 0x34;   // enable weak pullup on PA2,4,5
    GPIO_PORTA_DEN_R |= 0x34;   // enable digital I/O on PA2,4,5
                                // configure PA2,4,5 as SSI
    GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R & 0xFF00F0FF) + 0x00220200;
    GPIO_PORTA_AMSEL_R &= ~0x34; // disable analog functionality on PA2,4,5

    SYSCTL_RCGCSSI_R |= 0x01; // activate clock for SSI0
    while ((SYSCTL_PRSSI_R & 0x01) == 0)
    {
    }; // allow time for clock to stabilize
    SSI0_CR1_R &= ~SSI_CR1_SSE; // disable SSI
    SSI0_CR1_R &= ~SSI_CR1_MS;  // master mode
                                // clock divider for 8 MHz SSIClk (assumes 16 MHz PIOSC)
    SSI0_CPSR_R = (SSI0_CPSR_R & ~SSI_CPSR_CPSDVSR_M) + CPSDVSR;
    // CPSDVSR must be even from 2 to 254

    SSI0_CR0_R &= ~(SSI_CR0_SCR_M | // SCR = 0 (80 Mbps base clock)
                    SSI_CR0_SPH |   // SPH = 0
                    SSI_CR0_SPO);   // SPO = 0
                                    // FRF = Freescale format
    SSI0_CR0_R = (SSI0_CR0_R & ~SSI_CR0_FRF_M) + SSI_CR0_FRF_MOTO;
    // DSS = 8-bit data
    SSI0_CR0_R = (SSI0_CR0_R & ~SSI_CR0_DSS_M) + SSI_CR0_DSS_8;
    SSI0_CR1_R |= SSI_CR1_SSE; // enable SSI
}

#define FCLK_SLOW()                                              \
    {                                                            \
        SSI0_CPSR_R = (SSI0_CPSR_R & ~SSI_CPSR_CPSDVSR_M) + 200; \
    }
#define FCLK_FAST()                                            \
    {                                                          \
        SSI0_CPSR_R = (SSI0_CPSR_R & ~SSI_CPSR_CPSDVSR_M) + 8; \
    }

// de-asserts the CS pin to the card
#define SDC_DESELECT() SDC_CS = SDC_CS_HIGH;
// asserts the CS pin to the card
#define SDC_SELECT() SDC_CS = SDC_CS_LOW;
// de-asserts the CS pin to the IMU
#define IMU_DESELECT() IMU_CS = IMU_CS_HIGH;
// asserts the CS pin to the IMU
#define IMU_SELECT() IMU_CS = IMU_CS_LOW;
// de-asserts the CS pin to the DAC
#define DAC_DESELECT() DAC_CS = DAC_CS_HIGH;
// asserts the CS pin to the DAC
#define DAC_SELECT() DAC_CS = DAC_CS_LOW;
// de-asserts the CS pin to the TFT
#define TFT_DESELECT() TFT_CS = TFT_CS_HIGH;
// asserts the CS pin to the TFT
#define TFT_SELECT() TFT_CS = TFT_CS_LOW;

/* Exchange a byte */
// Inputs:  byte to be sent to SPI
// Outputs: byte received from SPI
// assumes it has been selected with CS low
static BYTE xchg_spi(BYTE dat)
{
    BYTE volatile rcvdat;
    // wait until SSI0 not busy/transmit FIFO empty
    while ((SSI0_SR_R & SSI_SR_BSY) == SSI_SR_BSY)
    {
    };
    SSI0_DR_R = dat; // data out
    while ((SSI0_SR_R & SSI_SR_RNE) == 0)
    {
    }; // wait until response
    rcvdat = SSI0_DR_R; // acknowledge response
    return rcvdat;
}

/* Receive byte */
// Inputs:  none
// Outputs: byte received from SPI
static BYTE rcvr_spi(void)
{
    // wait until SSI0 not busy/transmit FIFO empty
    while ((SSI0_SR_R & SSI_SR_BSY) == SSI_SR_BSY)
    {
    };
    SSI0_DR_R = 0xFF; // data out, garbage
    while ((SSI0_SR_R & SSI_SR_RNE) == 0)
    {
    }; // wait until response
    return (BYTE)SSI0_DR_R; // read received data
}

/* Receive multiple byte */
// Input:  buff Pointer to empty buffer into which data will be received
//         btr  Number of bytes to receive (even number)
// Output: none
static void rcvr_spi_multi(BYTE *buff, UINT btr)
{
    while (btr)
    {
        *buff = rcvr_spi(); // return by reference
        btr--;
        buff++;
    }
}

/* Send multiple byte */
// Input:  buff Pointer to the data which will be sent
//         btx  Number of bytes to send (even number)
// Output: none
static void xmit_spi_multi(const BYTE *buff, UINT btx)
{
    BYTE volatile rcvdat;
    while (btx)
    {
        SSI0_DR_R = *buff; // data out
        while ((SSI0_SR_R & SSI_SR_RNE) == 0)
        {
        }; // wait until response
        rcvdat = SSI0_DR_R; // acknowledge response
        btx--;
        buff++;
    }
}

/* wait for ready */
// Input:  time to wait in ms
// Output: 1:Ready, 0:Timeout
static int wait_ready(UINT wt){
  BYTE d;
  Timer2 = wt;
  do {
    d = xchg_spi(0xFF);
    /* This loop takes a time. Insert rot_rdq() here for multitask environment. */
  } while (d != 0xFF && Timer2);  /* Wait for ready or timeout */
  return (d == 0xFF) ? 1 : 0;
}

/* deselect sdc and release spi*/
static void deselect_SDC(void)
{
    SDC_DESELECT();      /* CS = H */
    xchg_spi(0xFF); /* Dummy clock (force DO hi-z for multiple slave SPI) */
}

/* deselect imu and release spi*/
static void deselect_IMU(void)
{
    IMU_DESELECT();      /* CS = H */
    xchg_spi(0xFF); /* Dummy clock (force DO hi-z for multiple slave SPI) */
}

/* deselect dac and release spi*/
static void deselect_DAC(void)
{
    DAC_DESELECT();      /* CS = H */
    xchg_spi(0xFF); /* Dummy clock (force DO hi-z for multiple slave SPI) */
}

/* deselect tft and release spi*/
static void deselect_TFT(void)
{
    TFT_DESELECT();      /* CS = H */
    xchg_spi(0xFF); /* Dummy clock (force DO hi-z for multiple slave SPI) */
}

/* select SDC*/
// Input:  none
// Output: 1:OK, 0:Timeout in 500ms
static int select_SDC(void)
{
    IMU_CS = IMU_CS_HIGH; // make sure IMU is off
    DAC_CS = DAC_CS_HIGH; // make sure DAC is off
    TFT_CS = TFT_CS_HIGH; // make sure TFT is off
    SDC_SELECT();
    xchg_spi(0xFF); /* Dummy clock (force DO enabled) */
    if (wait_ready(500))
        return 1; /* OK */
    SDC_DESELECT();
    return 0; /* Timeout */
}

/* select TFT*/
// Input:  none
// Output: 1:OK, 0:Timeout in 500ms
static int select_TFT(void)
{
    IMU_CS = IMU_CS_HIGH; // make sure IMU is off
    DAC_CS = DAC_CS_HIGH; // make sure DAC is off
    SDC_CS = SDC_CS_HIGH; // make sure SDC is off
    TFT_SELECT();
    xchg_spi(0xFF); /* Dummy clock (force DO enabled) */
    if (wait_ready(500))
        return 1; /* OK */
    TFT_DESELECT();
    return 0; /* Timeout */
}

/* select IMU*/
// Input:  none
// Output: 1:OK, 0:Timeout in 500ms
static int select_IMU(void)
{
    SDC_CS = SDC_CS_HIGH; // make sure IMU is off
    DAC_CS = DAC_CS_HIGH; // make sure DAC is off
    TFT_CS = TFT_CS_HIGH; // make sure TFT is off
    IMU_SELECT();
    xchg_spi(0xFF); /* Dummy clock (force DO enabled) */
    if (wait_ready(500))
        return 1; /* OK */
    IMU_DESELECT();
    return 0; /* Timeout */
}

/* select DAC*/
// Input:  none
// Output: 1:OK, 0:Timeout in 500ms
static int select_DAC(void)
{
    IMU_CS = IMU_CS_HIGH; // make sure IMU is off
    SDC_CS = SDC_CS_HIGH; // make sure DAC is off
    TFT_CS = TFT_CS_HIGH; // make sure TFT is off
    DAC_SELECT();
    xchg_spi(0xFF); /* Dummy clock (force DO enabled) */
    if (wait_ready(500))
        return 1; /* OK */
    DAC_DESELECT();
    return 0; /* Timeout */
}




