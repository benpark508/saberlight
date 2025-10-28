#include "BetterSwitch.h"

// Global Flags
volatile bool PE0_flag = false;
volatile bool PE1_flag = false;
volatile bool PF4_flag = false;

volatile uint32_t PE0_start = 0;
volatile uint32_t PE1_start = 0;
volatile uint32_t PF4_start = 0;

// GPIO Switch Configurations

GPIOConfig_t PE0Config = {
    .portBase   = GPIO_PORTE_BASE,
    .pin        = 0,
    .direction  = GPIO_DIR_INPUT,
    .pull       = GPIO_PULL_DOWN,
    .openDrain  = false,
    .analog     = false,
    .altFunc    = false,
    .altFuncNum = 0,
    .intMode    = GPIO_INT_RISING
};

GPIOConfig_t PE1Config = {
    .portBase   = GPIO_PORTE_BASE,
    .pin        = 1,
    .direction  = GPIO_DIR_INPUT,
    .pull       = GPIO_PULL_DOWN,
    .openDrain  = false,
    .analog     = false,
    .altFunc    = false,
    .altFuncNum = 0,
    .intMode    = GPIO_INT_RISING
};

GPIOConfig_t PF4Config = {
    .portBase   = GPIO_PORTF_BASE,
    .pin        = 4,
    .direction  = GPIO_DIR_INPUT,
    .pull       = GPIO_PULL_UP,
    .openDrain  = false,
    .analog     = false,
    .altFunc    = false,
    .altFuncNum = 0,
    .intMode    = GPIO_INT_FALLING
};


void PE0_Handler(void) {
    GPIO_ClearInterrupt(GPIO_PORTE_BASE, 0);
    GPIO_DisableInterrupt(GPIO_PORTE_BASE, 0);
    PE0_flag = true;
    PE0_start = ticks_ms;
}
void PE1_Handler(void) {
    GPIO_ClearInterrupt(GPIO_PORTE_BASE, 1);
    GPIO_DisableInterrupt(GPIO_PORTE_BASE, 1);
    PE1_flag = true;
    PE1_start = ticks_ms;
}

void PF4_Handler(void) {
    GPIO_ClearInterrupt(GPIO_PORTF_BASE, 4);
    GPIO_DisableInterrupt(GPIO_PORTF_BASE, 4);
    PF4_flag = true;
    PF4_start = ticks_ms;
}

void BetterSwitch_Init() {
    GPIO_Init(&PE0Config);
    GPIO_AttachISR(&PE0Config, PE0_Handler);
    GPIO_Init(&PE1Config);
    GPIO_AttachISR(&PE1Config, PE1_Handler);
    GPIO_Init(&PF4Config);
    GPIO_AttachISR(&PF4Config, PF4_Handler);
}