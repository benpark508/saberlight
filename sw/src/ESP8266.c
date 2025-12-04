// ESP8266.c
// Hardware driver for Custom PCB
// Handles Power (PA0), Boot Mode (PE1), and Reset (PC6)

#include "ESP8266.h"
#include "../inc/tm4c123gh6pm.h"

static void Delay_Internal(uint32_t time){
    while(time){ time--; }
}

void ESP8266_Init(void){
    volatile uint32_t delay;
    
    // ============================================
    // 1. ENABLE ESP POWER (PA0 -> CH_PD)
    // ============================================
    SYSCTL_RCGCGPIO_R |= 0x01; // Port A Clock
    delay = SYSCTL_RCGCGPIO_R;
    
    // Configure PA0 as GPIO Output
    GPIO_PORTA_AMSEL_R &= ~0x01;  // Disable Analog
    GPIO_PORTA_PCTL_R  &= ~0x0000000F; // Clear PCTL (GPIO)
    GPIO_PORTA_DIR_R   |= 0x01;   // Output
    GPIO_PORTA_AFSEL_R &= ~0x01;  // Regular GPIO
    GPIO_PORTA_DEN_R   |= 0x01;   // Digital Enable
    
    // TURN ON THE ESP (PA0 High)
    GPIO_PORTA_DATA_R  |= 0x01;   


    // ============================================
    // 2. CONFIGURE UART + RESET (Port C)
    // ============================================
    // PC4=Rx, PC5=Tx, PC6=RST, PC7=RDY
    SYSCTL_RCGCGPIO_R |= 0x04;      
    delay = SYSCTL_RCGCGPIO_R;      
    
    // JTAG Safety: Clear bits 4-7 only
    GPIO_PORTC_AMSEL_R &= ~0xF0; 
    
    // PCTL: Set PC4/5 to UART(1), PC6/7 to GPIO(0)
    GPIO_PORTC_PCTL_R = (GPIO_PORTC_PCTL_R & 0xFF00FFFF) + 0x00110000;
    
    // Directions: PC5(Tx) Out, PC6(RST) Out, PC4/7 In
    GPIO_PORTC_DIR_R = (GPIO_PORTC_DIR_R & ~0xF0) | 0x60; 
    
    // Alt Func: PC4/5 Only
    GPIO_PORTC_AFSEL_R = (GPIO_PORTC_AFSEL_R & ~0xF0) | 0x30;
    
    // Digital Enable
    GPIO_PORTC_DEN_R |= 0xF0;
    
    // Set RST (PC6) High (Run)
    GPIO_PORTC_DATA_R |= 0x40; 


    // ============================================
    // 3. CONFIGURE BOOT MODE (Port E: PE1 -> GPIO0)
    // ============================================
    SYSCTL_RCGCGPIO_R |= 0x10; 
    delay = SYSCTL_RCGCGPIO_R;
    
    GPIO_PORTE_AMSEL_R &= ~0x02; 
    GPIO_PORTE_PCTL_R  &= ~0x000000F0; 
    GPIO_PORTE_DIR_R   |= 0x02;  // PE1 Output
    GPIO_PORTE_AFSEL_R &= ~0x02; 
    GPIO_PORTE_DEN_R   |= 0x02;  
    
    // Force "Run Mode" (PE1 High)
    GPIO_PORTE_DATA_R |= 0x02;   
    
    // Wait a moment for power to stabilize
    Delay_Internal(200000);
}

void ESP8266_Reset(void){
    // Reinforce Power (PA0) and Mode (PE1)
    GPIO_PORTA_DATA_R |= 0x01; 
    GPIO_PORTE_DATA_R |= 0x02;
    
    // Pulse Reset (PC6)
    GPIO_PORTC_DATA_R &= ~0x40; 
    Delay_Internal(80000); 
    GPIO_PORTC_DATA_R |= 0x40;  
    Delay_Internal(16000000); // 1-2 sec boot wait
}