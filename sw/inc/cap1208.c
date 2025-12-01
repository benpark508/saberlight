// cap1208.c
// Runs on TM4C123
// Driver for cap1208 capacitive touch sensor
// Ben Park
// October 28 2025

#include <stdint.h>
#include "../inc/cap1208.h"
#include "../inc/SysTick.h"
#include "../inc/GPIO_HAL.h"

uint8_t inputStatus = 0;

GPIOConfig_t PF4Config = {
    .portBase   = GPIO_PORTF_BASE,
    .pin        = 4,
    .direction  = GPIO_DIR_INPUT,
    .pull       = false,
    .openDrain  = false,
    .analog     = false,
    .altFunc    = false,
    .altFuncNum = 0,
    .intMode    = GPIO_INT_FALLING
};

GPIO_PORT_ISR(4, GPIOPortF_Handler)

// read input status register
// output: bitmask of inputs currently being touched
// bit 0 = channel 1, bit 1 = channel 2, ..., bit 7 = channel 8
void CAP1208_ReadInputs(uint8_t *status)
{
    I2C3_BlockRead(CAP1208_ADDRESS, R_INPUT_STATUS, status, 1);
}

// call to clear interrupt flag
void CAP1208_ClearINT(void)
{
    I2C3_BlockWrite(CAP1208_ADDRESS, R_MAIN_CONTROL, (uint8_t[]){0x00}, 1); // clear INT bit; this clears sensor input registers
}

// sensitivity: 0 (highest) 128x to 7 (lowest) 1x
void CAP1208_SetSensitivity(uint8_t sensitivity)
{
    uint8_t value = ((sensitivity & 0x07) << 4) | 0b1111; // base shift default
    I2C3_BlockWrite(CAP1208_ADDRESS, R_SENSITIVITY, &value, 1);
}

// enable: bitmask of inputs to enable (1 = enable, 0 = disable)
// bit 0 = channel 1, bit 1 = channel 2, ..., bit 7 = channel 8
void CAP1208_EnableInputs(uint8_t enable)
{
    I2C3_BlockWrite(CAP1208_ADDRESS, R_INPUT_ENABLE, &enable , 1);
}

// threshold is 7 bits: 0 (least sensitive) to 127 (most sensitive)
// channel: 1 to 8
void CAP1208_SetThreshold(uint8_t channel, uint8_t threshold)
{
    if(channel < 1 || channel > 8) return; // invalid channel
    uint8_t reg = R_INPUT_1_THRESH + (channel - 1);
    I2C3_BlockWrite(CAP1208_ADDRESS, reg, &threshold, 1);
}

void CAP1208_Calibrate(void)
{
    I2C3_BlockWrite(CAP1208_ADDRESS, R_CALIBRATION, (uint8_t[]){0b11111111}, 1); // write 1 to start calibration
}

void CAP1208_EnableInterrupts(uint8_t enable)
{
    I2C3_BlockWrite(CAP1208_ADDRESS, R_INTERRUPT_EN, &enable, 1);
}

// returns count for specified channel (1 to 8)
void CAP1208_ReadCount(uint8_t channel, int8_t *count)
{
    uint8_t temp;
    I2C3_BlockRead(CAP1208_ADDRESS, R_INPUT_1_DELTA + (channel - 1), &temp, 1);
    *count = (int8_t)temp; 
}

void Input_Handler(void) {
    GPIO_ClearInterrupt(GPIO_PORTF_BASE, 4);
    CAP1208_ReadInputs(&inputStatus);
    CAP1208_ClearINT();
}

void CAP1208_Init(void)
{
    I2C3_Init(400000, 80000000); // Initialize I2C3 at 400kHz with 80MHz bus
    GPIO_Init(&PF4Config);
    GPIO_AttachISR(&PF4Config, Input_Handler);
    CAP1208_ClearINT(); // Clear any existing interrupts, active power mode
    I2C3_BlockWrite(CAP1208_ADDRESS, R_CONFIGURATION2, (uint8_t[]){0b01000000}, 1); //only interrupt on press
    //I2C3_BlockWrite(CAP1208_ADDRESS, R_REPEAT_EN, (uint8_t[]){0x00}, 1); // Disable repeat for all inputs
    CAP1208_SetSensitivity(3); // Set highest sensitivity, 128x
    CAP1208_EnableInputs(0xFF); // Enable all 8 inputs
    CAP1208_Calibrate(); // Calibrate all inputs
    SysTick80_Wait10ms(30); // wait 300ms for calibration to complete
    CAP1208_SetThreshold(1, 0);
    CAP1208_ClearINT(); // Clear any existing interrupts, active power mode
    CAP1208_EnableInterrupts(0xFF); // Enable interrupts for all inputs
    GPIO_EnableInterrupt(GPIO_PORTF_BASE, 4);
}

void CAP1208_GetInputs(uint8_t *result)
{
    *result = inputStatus;
}