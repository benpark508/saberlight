// UART4.c
// Interrupt-driven UART Driver with Software FIFO
// Runs on TM4C123

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "UART4.h"

#define FIFO_SIZE 64  // Size of software buffer

long StartCritical(void);     // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value

// --- FIFO Implementation ---
static char RxFifo[FIFO_SIZE];
static volatile uint32_t RxPutI; // Index to put new data
static volatile uint32_t RxGetI; // Index to get data
static volatile uint32_t RxFifoCount; // Number of items in FIFO

void Fifo_Init(void){
    RxPutI = RxGetI = RxFifoCount = 0;
}

int Fifo_Put(char data){
    if(RxFifoCount == FIFO_SIZE){
        return 0; // FIFO Full (Fail)
    }
    RxFifo[RxPutI] = data;
    RxPutI = (RxPutI + 1) % FIFO_SIZE;
    
    // Critical section for counter
    long sr = StartCritical();
    RxFifoCount++;
    EndCritical(sr);
    return 1;
}

void UART4_Flush(void){
    long sr = StartCritical();
    
    // 1. Reset Software FIFO
    Fifo_Init();
    
    // 2. Drain Hardware FIFO
    // Loop until the "Receive FIFO Empty" flag (Bit 4) is set
    while((UART4_FR_R & 0x10) == 0){
        volatile uint32_t trash = UART4_DR_R; // Read and discard
    }
    
    EndCritical(sr);
}

int Fifo_Get(char *datapt){
    if(RxFifoCount == 0){
        return 0; // FIFO Empty (Fail)
    }
    *datapt = RxFifo[RxGetI];
    RxGetI = (RxGetI + 1) % FIFO_SIZE;
    
    // Critical section for counter
    long sr = StartCritical();
    RxFifoCount--;
    EndCritical(sr);
    return 1;
}

// --- UART4 Implementation ---

void UART4_Init(void){
    Fifo_Init();

    // 1. Clock Setup
    SYSCTL_RCGCUART_R |= 0x10; // UART4
    SYSCTL_RCGCGPIO_R |= 0x04; // Port C
    
    // Wait for clocks
    volatile uint32_t delay = SYSCTL_RCGCGPIO_R;
    
    // 2. GPIO Setup (PC4=Rx, PC5=Tx)
    GPIO_PORTC_AMSEL_R &= ~0x30;
    GPIO_PORTC_PCTL_R = (GPIO_PORTC_PCTL_R & 0xFF00FFFF) + 0x00110000;
    GPIO_PORTC_DIR_R |= 0x20;  // PC5 Output
    GPIO_PORTC_DIR_R &= ~0x10; // PC4 Input
    GPIO_PORTC_AFSEL_R |= 0x30;
    GPIO_PORTC_DEN_R |= 0x30;

    // 3. UART Setup
    UART4_CTL_R &= ~0x01;    // Disable UART
    UART4_IBRD_R = 520;      // 9600 Baud (Assuming 80MHz)
    UART4_FBRD_R = 53;
    UART4_LCRH_R = 0x70;     // 8-bit, FIFO enable
    
    // 4. INTERRUPT SETUP
    UART4_IM_R |= 0x10;      // Enable RX Interrupt (Bit 4)
    
    // UART4 is IRQ 60 (Vector 76)
    // Priority 2 (Bits 13-15 of PRI15)
    NVIC_PRI15_R = (NVIC_PRI15_R & 0xFF00FFFF) | 0x400000; 
    NVIC_EN1_R = 1 << 28;    // Enable IRQ 60 (Bit 28 of EN1)

    UART4_CTL_R |= 0x01;     // Enable UART
}

// The ISR - This runs automatically when data arrives
void UART4_Handler(void){
    // Check if RX interrupt occurred
    if(UART4_RIS_R & 0x10){
        UART4_ICR_R = 0x10; // Acknowledge/Clear flag
        
        // Read all available bytes from Hardware FIFO
        while((UART4_FR_R & 0x10) == 0){ // While RXFE is 0 (FIFO not empty)
            char c = (char)(UART4_DR_R & 0xFF);
            Fifo_Put(c); // Put into software buffer
        }
    }
}

// Non-blocking check for input
int UART4_GetPacket(char *data){
    return Fifo_Get(data);
}

// Basic output (Polled is fine for TX usually, unless sending massive data)
void UART4_OutChar(char data){
    while((UART4_FR_R & 0x20) != 0); // Wait for TXFF
    UART4_DR_R = data;
}