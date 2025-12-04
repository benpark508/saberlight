// -------------------------------------------------
// UART4.h
// Runs on TM4C123
// Simple device driver for the UART4
//
// Modified for UART4 (PC4/PC5)
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
#define LR 0x0A

// initialize index FIFO
void UART4_Fifo_Init(uint32_t size);
    
//------------   UART4_InChar   ------------
// Wait for new serial port input
// Input: none
// Output: ASCII code for key typed
char UART4_InChar(void);

//------------UART4_OutChar------------
// Output 8-bit to serial port
// Input: letter is an 8-bit ASCII character to be transferred
// Output: none
void UART4_OutChar(char data);

//------------------- UART4_Init-------------------
// Initializes uart and gpio needed
// Configure UART4 for serial full duplex operation
// Inputs: none
// Outputs: none
void UART4_Init(void);

//----------UART4_FIFOtoBuffer----------
// - copies uart fifo to RXBuffer
// Inputs: none
// Outputs:none
void UART4_FIFOtoBuffer(void);

//----------UART4_GetString------------
// - gets data from software UART4 FIFO (in a string format)
// Inputs: pointer to empty buffer
// Outputs: Null terminated string
void UART4_GetString(unsigned char *bufPt);

//------------UART4_InUDec------------
// InUDec accepts ASCII input in unsigned decimal format
//     and converts to a 32-bit unsigned number
//     valid range is 0 to 4294967295 (2^32-1)
// Input: none
// Output: 32-bit unsigned number
// If you enter a number above 4294967295, it will return an incorrect value
// Backspace will remove last digit typed
uint32_t UART4_InUDec(void);

//-----------------------UART4_OutUDec-----------------------
// Output a 32-bit number in unsigned decimal format
// Input: 32-bit number to be transferred
// Output: none
// Variable format 1-10 digits with no space before or after
void UART4_OutUDec(uint32_t n);

//---------------------UART4_InUHex----------------------------------------
// Accepts ASCII input in unsigned hexadecimal (base 16) format
// Input: none
// Output: 32-bit unsigned number
// No '$' or '0x' need be entered, just the 1 to 8 hex digits
// It will convert lower case a-f to uppercase A-F
//     and converts to a 16 bit unsigned number
//     value range is 0 to FFFFFFFF
// If you enter a number above FFFFFFFF, it will return an incorrect value
// Backspace will remove last digit typed
uint32_t UART4_InUHex(void);

//--------------------------UART4_OutUHex----------------------------
// Output a 32-bit number in unsigned hexadecimal format
// Input: 32-bit number to be transferred
// Output: none
// Variable format 1 to 8 digits with no space before or after
void UART4_OutUHex(uint32_t number);

//------------UART4_OutString------------
// Output String (NULL termination)
// Input: pointer to a NULL-terminated string to be transferred
// Output: none
void UART4_OutString(char *pt);

//------------UART4_InString------------
// Accepts ASCII characters from the serial port
//    and adds them to a string until <enter> is typed
//    or until max length of the string is reached.
// It echoes each character as it is inputted.
// If a backspace is inputted, the string is modified
//    and the backspace is echoed
// terminates the string with a null character
// uses busy-waiting synchronization on RDRF
// Input: pointer to empty buffer, size of buffer
// Output: Null terminated string
// -- Modified by Agustinus Darmawan + Mingjie Qiu --
void UART4_InString(char *bufPt, uint16_t max);

// Helper to print new line
void UART4_Out_CRLF(void);

// Test function
void Test_UART4(void);

#endif // UART4_H