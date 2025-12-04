// -------------------------------------------------
// UART4.h
// Runs on TM4C123
// Simple device driver for the UART4
//
// Modified for UART4 (PC4/PC5)
// Matching interrupt-driven implementation
//------------------------------------------------------

#ifndef UART4_H
#define UART4_H

#include <stdio.h>
#include <stdint.h>
#include "inc/tm4c123gh6pm.h"

// standard ASCII symbols
#define CR   0x0D
#define LF   0x0A
#define BS   0x08
#define ESC  0x1B
#define SP   0x20
#define DEL  0x7F
#define LR   0x0A

//------------------- UART4_Init-------------------
// Initializes UART4 and GPIO PC4, PC5
// Configures UART4 for 9600 baud rate (assuming 80MHz clock), 
// 8 bit word length, no parity bits, one stop bit, FIFOs enabled
// Enables RX interrupts
// Inputs: none
// Outputs: none
void UART4_Init(void);

//------------UART4_OutChar------------
// Output 8-bit to serial port
// Input: letter is an 8-bit ASCII character to be transferred
// Output: none
void UART4_OutChar(char data);

//------------UART4_GetPacket------------
// Non-blocking check for new serial port input
// Reads one character from the software FIFO
// Input: pointer to buffer to store data
// Output: 1 if data was read, 0 if FIFO was empty
int UART4_GetPacket(char *data);

//------------UART4_Handler------------
// Interrupt Handler for UART4
// Handles RX interrupts and moves data from Hardware FIFO to Software FIFO
// Input: none
// Output: none
void UART4_Handler(void);

// --------------------------------------------------
// Utility Functions (Prototypes)
// Note: These need implementation in UART4.c if used
// --------------------------------------------------

//------------UART4_OutString------------
// Output String (NULL termination)
// Input: pointer to a NULL-terminated string to be transferred
// Output: none
void UART4_OutString(char *pt);

//------------UART4_InUDec------------
// InUDec accepts ASCII input in unsigned decimal format
//     and converts to a 32-bit unsigned number
// Input: none
// Output: 32-bit unsigned number
uint32_t UART4_InUDec(void);

//-----------------------UART4_OutUDec-----------------------
// Output a 32-bit number in unsigned decimal format
// Input: 32-bit number to be transferred
// Output: none
void UART4_OutUDec(uint32_t n);

//---------------------UART4_InUHex----------------------------------------
// Accepts ASCII input in unsigned hexadecimal (base 16) format
// Input: none
// Output: 32-bit unsigned number
uint32_t UART4_InUHex(void);

//--------------------------UART4_OutUHex----------------------------
// Output a 32-bit number in unsigned hexadecimal format
// Input: 32-bit number to be transferred
// Output: none
void UART4_OutUHex(uint32_t number);

// Helper to print new line
void UART4_Out_CRLF(void);

#endif // UART4_H