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


typedef signed int		INT;
typedef unsigned int	UINT;

/* These types are assumed as 8-bit integer */
typedef signed char CHAR;
typedef unsigned char UCHAR;
typedef unsigned char BYTE;

/* These types are assumed as 16-bit integer */
typedef signed short SHORT;
typedef unsigned short USHORT;
typedef unsigned short WORD;

/* These types are assumed as 32-bit integer */
typedef signed long LONG;
typedef unsigned long ULONG;
typedef unsigned long DWORD;

/**
 * \brief Boolean type
 */
typedef enum
{
    FALSE = 0,
    TRUE
} BOOL;

void SPI_Init(unsigned long CPSDVSR);

static BYTE xchg_spi(BYTE dat);
static BYTE rcvr_spi(void);
static void rcvr_spi_multi(BYTE *buff, UINT btr);
static void xmit_spi_multi(const BYTE *buff, UINT btx);
static int wait_ready(UINT wt);

static int select_SDC(void);
static int select_TFT(void);
static int select_IMU(void);
static int select_DAC(void);
static void deselect_SDC(void);
static void deselect_TFT(void);
static void deselect_IMU(void);
static void deselect_DAC(void);