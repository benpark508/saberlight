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
#include "../inc/SPI.h"

static volatile UINT Timer1, Timer2;

// initialize for SPI
// assumes: system clock rate is 80 MHz, SCR=0
// SSIClk = SysClk / (CPSDVSR * (1 + SCR)) = 80 MHz/CPSDVSR
// 200 for    400,000 bps slow mode, used during initialization
// 8   for 10,000,000 bps fast mode, used during disk I/O
void SPI_Init(unsigned long CPSDVSR)
{
    volatile uint32_t delay;

    SYSCTL_RCGCGPIO_R |= 0x01; // activate port A
    while ((SYSCTL_PRGPIO_R & 0x01) == 0)
    {
    }; // allow time for clock to start

    SYSCTL_RCGCSSI_R |= 0x01; // activate SSI0

    SYSCTL_RCGCGPIO_R |= 0x08; // activate port D
    while ((SYSCTL_PRGPIO_R & 0x08) == 0)
    {
    };

    // initialize IMU CS
    GPIO_PORTD_DIR_R |= 0x10;         // PD4 Output
    GPIO_PORTD_AFSEL_R &= ~0x10;      // GPIO
    GPIO_PORTD_DEN_R |= 0x10;         // PD4 Digital Enable
    GPIO_PORTD_AMSEL_R &= ~0x10;      // disable analog functionality on PD4
    GPIO_PORTD_PCTL_R &= ~0x000F0000; // PD4 as GPIO
    IMU_CS = IMU_CS_HIGH;             // deselect imu

    GPIO_PORTD_PUR_R |= 0x10;

    // initialize TFT CS
    GPIO_PORTA_DIR_R |= 0x40;         // PA6 = CS
    GPIO_PORTA_AFSEL_R &= ~0x40;      // GPIO
    GPIO_PORTA_DEN_R |= 0x40;         // digital enable
    GPIO_PORTA_AMSEL_R &= ~0x40;      // no analog
    GPIO_PORTA_PCTL_R &= ~0x0F000000; // PA6 as GPIO
    TFT_CS = TFT_CS_HIGH;             // deselect TFT

    GPIO_PORTA_PUR_R |= 0x40;

    // initialize DAC CS
    GPIO_PORTA_DIR_R |= 0x02;         // PA1 output
    GPIO_PORTA_AFSEL_R &= ~0x02;      // GPIO
    GPIO_PORTA_DEN_R |= 0x02;         // PA1 digital enable
    GPIO_PORTA_AMSEL_R &= ~0x02;      // disable analog functionality on PA1
    GPIO_PORTA_PCTL_R &= ~0x000000F0; // PA1 as GPIO
    DAC_CS = DAC_CS_HIGH;             // deselect dac

    GPIO_PORTA_PUR_R |= 0x02;

    // initialize SDC CS
    GPIO_PORTA_PUR_R |= 0x80;  // enable weak pullup on PA7
    GPIO_PORTA_DIR_R |= 0x80;  // make PA7 output
    GPIO_PORTA_DR4R_R |= 0x80; // 4mA output on outputs
    SDC_CS = SDC_CS_HIGH;      // deselect SDC
    GPIO_PORTA_PCTL_R &= ~0xF0000000;
    GPIO_PORTA_AMSEL_R &= ~0x80; // disable analog functionality on PA7
    GPIO_PORTA_DEN_R |= 0x80;    // enable digital I/O on PA7

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

    volatile uint32_t dump;
    while (SSI0_SR_R & SSI_SR_RNE)
    {
        dump = SSI0_DR_R;
    }
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
BYTE xchg_spi(BYTE dat)
{
    BYTE volatile rcvdat;

    while ((SSI0_SR_R & SSI_SR_RNE) != 0)
    {
        rcvdat = SSI0_DR_R;
    }
    // wait until SSI0 not busy/transmit FIFO empty
    while ((SSI0_SR_R & SSI_SR_TNF) == 0)
    {
    } // wait until transmit FIFO is not full
    {};
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
BYTE rcvr_spi(void)
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
void rcvr_spi_multi(BYTE *buff, UINT btr)
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
void xmit_spi_multi(const BYTE *buff, UINT btx)
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
int wait_ready(UINT wt)
{
    BYTE d;
    Timer2 = wt;
    do
    {
        d = xchg_spi(0xFF);
        /* This loop takes a time. Insert rot_rdq() here for multitask environment. */
    } while (d != 0xFF && Timer2); /* Wait for ready or timeout */
    return (d == 0xFF) ? 1 : 0;
}

/* deselect sdc and release spi*/
void deselect_SDC(void)
{
    SDC_DESELECT(); /* CS = H */
    xchg_spi(0xFF); /* REQUIRED: Dummy clock to force SD card to release MISO */
}

/* deselect imu and release spi*/
void deselect_IMU(void)
{
    IMU_DESELECT(); /* CS = H */
    WAIT_SSI0_IDLE();
}

/* deselect dac and release spi*/
void deselect_DAC(void)
{
    DAC_DESELECT(); /* CS = H */
    WAIT_SSI0_IDLE();
}

/* deselect tft and release spi*/
void deselect_TFT(void)
{
    TFT_DESELECT(); /* CS = H */
    WAIT_SSI0_IDLE();
}

/* select SDC*/
// Input:  none
// Output: 1:OK, 0:Timeout in 500ms
int select_SDC(void)
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
int select_TFT(void)
{
    while ((SSI0_SR_R & SSI_SR_BSY) == SSI_SR_BSY)
    {
    };
    IMU_CS = IMU_CS_HIGH; // make sure IMU is off
    DAC_CS = DAC_CS_HIGH; // make sure DAC is off
    SDC_CS = SDC_CS_HIGH; // make sure SDC is off
    TFT_SELECT();
    return 1;
}

/* select IMU*/
// Input:  none
// Output: 1:OK, 0:Timeout in 500ms
int select_IMU(void)
{
    while ((SSI0_SR_R & SSI_SR_BSY) == SSI_SR_BSY)
    {
    };
    SDC_CS = SDC_CS_HIGH; // make sure SDC is off
    DAC_CS = DAC_CS_HIGH; // make sure DAC is off
    TFT_CS = TFT_CS_HIGH; // make sure TFT is off
    IMU_SELECT();
    return 1;
}

/* select DAC*/
// Input:  none
// Output: 1:OK, 0:Timeout in 500ms
int select_DAC(void)
{
    while ((SSI0_SR_R & SSI_SR_BSY) == SSI_SR_BSY)
    {
    };
    IMU_CS = IMU_CS_HIGH; // make sure IMU is off
    SDC_CS = SDC_CS_HIGH; // make sure SDC is off
    TFT_CS = TFT_CS_HIGH; // make sure TFT is off
    DAC_SELECT();
    return 1;
}
