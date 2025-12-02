/*
 * @file      SPI.h
 * @brief     low-level SPI driver
 * @details   driver for LCD, SD Card, IMU, DAC initialization with SSI0
 * @version   V1.0
 * @author    Valvano
 * @copyright Copyright 2017 by Jonathan W. Valvano, valvano@mail.utexas.edu,
 * @warning   AS-IS
 * @note      For more information see  http://users.ece.utexas.edu/~valvano/
 * @date      December 1, 2025

 *****************************************************************************
*/

#include "../inc/integer.h"

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

#define FCLK_SLOW()                                              \
    {                                                            \
        SSI0_CPSR_R = (SSI0_CPSR_R & ~SSI_CPSR_CPSDVSR_M) + 200; \
    }
#define FCLK_FAST()                                            \
    {                                                          \
        SSI0_CPSR_R = (SSI0_CPSR_R & ~SSI_CPSR_CPSDVSR_M) + 8; \
    }
#define FCLK_LCD()                                              \
    {                                                           \
        SSI0_CPSR_R = (SSI0_CPSR_R & ~SSI_CPSR_CPSDVSR_M) + 10; \
    }


/**
 * \brief Boolean type
 */
typedef enum
{
    FALSE = 0,
    TRUE
} BOOL;

void SPI_Init(unsigned long CPSDVSR);

BYTE xchg_spi(BYTE dat);
BYTE rcvr_spi(void);
void rcvr_spi_multi(BYTE *buff, UINT btr);
void xmit_spi_multi(const BYTE *buff, UINT btx);
int wait_ready(UINT wt);

int select_SDC(void);
int select_TFT(void);
int select_IMU(void);
int select_DAC(void);
void deselect_SDC(void);
void deselect_TFT(void);
void deselect_IMU(void);
void deselect_DAC(void);