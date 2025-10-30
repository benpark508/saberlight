// I2C1.c
// Runs on TM4C123
// Busy-wait device driver for the I2C1.
// Daniel and Jonathan Valvano
// Jan 2, 2021
// This file originally comes from the TIDA-010021 Firmware (tidcf48.zip) and
// was modified by Pololu to support the MSP432P401R. Modified again for TM4C123

/* This example accompanies the books
   "Embedded Systems: Introduction to ARM Cortex M Microcontrollers",
      ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2020
   "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
      ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2024
   "Embedded Systems: Real-Time Operating Systems for ARM Cortex-M Microcontrollers",
      ISBN: 978-1466468863, Jonathan Valvano, copyright (c) 2020

 Copyright 2024 by Jonathan W. Valvano, valvano@mail.utexas.edu
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

// TM4C123 hardware
// SDA    PA7 I2C data
// SCL    PA6 I2C clock
/*
 *  I2C0 Conncection | I2C1 Conncection | I2C2 Conncection | I2C1 Conncection
 *  ---------------- | ---------------- | ---------------- | ----------------
 *  SCL -------- PB2 | SCL -------- PA6 | SCL -------- PE4 | SCL -------- PD0
 *  SDA -------- PB3 | SDA -------- PA7 | SDA -------- PE5 | SDA -------- PD1
 */
#include <stdint.h>
#include "../inc/I2C1.h"
#include "../inc/tm4c123gh6pm.h"
#define I2C_MCS_ACK             0x00000008  // Data Acknowledge Enable
#define I2C_MCS_DATACK          0x00000008  // Acknowledge Data
#define I2C_MCS_ADRACK          0x00000004  // Acknowledge Address
#define I2C_MCS_STOP            0x00000004  // Generate STOP
#define I2C_MCS_START           0x00000002  // Generate START
#define I2C_MCS_ERROR           0x00000002  // Error
#define I2C_MCS_RUN             0x00000001  // I2C Master Enable
#define I2C_MCS_BUSY            0x00000001  // I2C Busy
#define I2C_MCR_MFE             0x00000010  // I2C Master Function Enable
#define MAXRETRIES              5           // number of receive attempts before giving up

// let t be bus period, let F be bus frequency
// let f be I2C frequency
// at F=80 MHz, I2C period = (TPR+1)*250ns 
// f=400kHz,    I2C period = 20*(TPR+1)*12.5ns = 2.5us, with TPR=9
// I2C period, 1/f = 20*(TPR+1)*t 
// F/f = 20*(TPR+1)
// TPR = (F/f/20)-1 
void I2C1_Init(uint32_t I2Cfreq, uint32_t busFreq){
  SYSCTL_RCGCI2C_R |= 0x0002;           // activate I2C1
  SYSCTL_RCGCGPIO_R |= 0x0001;          // activate port A
  while((SYSCTL_PRGPIO_R&0x0001) == 0){};// ready?
  GPIO_PORTA_AFSEL_R |= 0xC0;           // 3) enable alt funct on PA7,6
  GPIO_PORTA_ODR_R |= 0x80;             // 4) enable open drain on PA7 only
  GPIO_PORTA_DR8R_R |= 0xC0;            //  high current on PA7,6
  GPIO_PORTA_DEN_R |= 0xC0;             // 5) enable digital I/O on PA7,6
                                        // 6) configure PA7,6 as I2C
  GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R&0x00FFFFFF)+0x33000000;
  GPIO_PORTA_AMSEL_R &= ~0xC0;          // 7) disable analog functionality on PA7,6
  I2C1_MCR_R = I2C_MCR_MFE;             // 9) master function enable, no glitch
  I2C1_MCR2_R = I2C_MCR2_GFPW_BYPASS;   // bypass glitch
// I2C1_MCR_R = I2C_MCR_MFE;          // 9) master function enable, glitch
// I2C1_MCR2_R = I2C_MCR2_GFPW_4; // 4 clock glitch
  I2C1_MTPR_R = ((busFreq/I2Cfreq)/20)-1; // 8) configure clock speed
}

static int I2C_wait_till_done(void)
{
    while(I2C1_MCS_R & I2C_MCS_BUSY); // wait until I2C master is not busy
    return I2C1_MCS_R & (I2C_MCS_DATACK + I2C_MCS_ADRACK + I2C_MCS_ERROR);  // return I2C error code
}

int I2C1_Write_Byte(int slave_address, char data)
{
    char error;

    I2C1_MSA_R = slave_address << 1;
    I2C1_MDR_R = data;             // write first byte
    I2C1_MCS_R = I2C_MCS_START + I2C_MCS_RUN + I2C_MCS_STOP; // 0x0000.0007
    error = I2C_wait_till_done();   // wait until write is complete
    if(error) return error;

    while(I2C1_MCS_R & I2C_MCS_BUSBSY);  // wait until bus is not busy
    error = I2C1_MCS_R & 0xE;  // I2C_MCS_DATACK + I2C_MCS_ADRACK + I2C_MCS_ERROR
    if(error) return error;

    return 0;       // no error
}

int I2C1_Read_Byte(int slave_address, uint8_t* data)
{
    char error;

    I2C1_MSA_R = slave_address << 1;
    I2C1_MSA_R |= I2C_MSA_RS;  // receive direction
    I2C1_MCS_R = I2C_MCS_START + I2C_MCS_RUN + I2C_MCS_STOP;  // 0x0000.0007
    error = I2C_wait_till_done();   // wait until read is complete
    if(error) return error;
    *data = I2C1_MDR_R;             // read byte

    while(I2C1_MCS_R & I2C_MCS_BUSBSY);  // wait until bus is not busy
    error = I2C1_MCS_R & 0xE;  // check error: I2C_MCS_DATACK + I2C_MCS_ADRACK + I2C_MCS_ERROR
    if(error) return error;

    return 0;       // no error
}

/** I2C Master transfer multiple bytes **/
int I2C1_Write_Buffer(int slave_address, int length, uint8_t* data)
{
    char error;
    if (length <= 0)
        return -1;
    /* send slave address and starting address */
    I2C1_MSA_R = slave_address << 1;
    I2C1_MDR_R = *data++;          // write first byte
    I2C1_MCS_R = I2C_MCS_START + I2C_MCS_RUN;
    error = I2C_wait_till_done();  // wait until write is complete
    if(error) return error;
    length--;

    /* send data one byte at a time */
    while (length > 1)
    {
        I2C1_MDR_R = *data++;   // write the next byte
        I2C1_MCS_R = I2C_MCS_RUN;
        error = I2C_wait_till_done();
        if (error) return error;
        length--;
    }

    /* send last byte and a STOP */
    I2C1_MDR_R = *data++;   //write the last byte
    I2C1_MCS_R = I2C_MCS_RUN + I2C_MCS_STOP;
    error = I2C_wait_till_done();
    if(error) return error;

    while(I2C1_MCS_R & I2C_MCS_BUSBSY);  // wait until bus is not busy

    return 0;
}

/** I2c Master read multiple bytes **/
int I2C1_Read_Buffer(int slave_address, int length, uint8_t* data)
{
    char error;

    if(length <= 0)
        return -1;

    I2C1_MSA_R = slave_address << 1;
    I2C1_MSA_R |= I2C_MSA_RS;  // receive direction

    if(length == 1)
        I2C1_MCS_R = I2C_MCS_ACK + I2C_MCS_START + I2C_MCS_RUN + I2C_MCS_STOP;
    else
        I2C1_MCS_R = I2C_MCS_ACK + I2C_MCS_START + I2C_MCS_RUN;
    error = I2C_wait_till_done();   // wait until operation is complete
    if(error) return error;
    *data++ = I2C1_MDR_R;           // read first byte

    if(length > 0)
    {
        /* read the remain bytes */
        while(length > 1)
        {
            I2C1_MCS_R = I2C_MCS_ACK + I2C_MCS_RUN;  // RUN
            error = I2C_wait_till_done();   // wait until operation is complete
            if(error) return error;
            *data++ = I2C1_MDR_R;           // read bytes
            length--;
        }

        I2C1_MCS_R = I2C_MCS_RUN + I2C_MCS_STOP;  // RUN+STOP
        error = I2C_wait_till_done();   // wait until operation is complete
        if(error) return error;
        *data++ = I2C1_MDR_R;           // read last byte
        length--;
    }

    while(I2C1_MCS_R & I2C_MCS_BUSBSY);  // wait until bus is not busy

    return 0;

}
