#ifndef GPIO_HAL_H
#define GPIO_HAL_H

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_gpio.h"
#include "inc/hw_types.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_nvic.h"
#include "../inc/tm4c123gh6pm.h"

typedef void (*GPIOHandler_t)(void);

// Direction
typedef enum {
    GPIO_DIR_INPUT,
    GPIO_DIR_OUTPUT
} GPIODir_t;

// Pull configuration
typedef enum {
    GPIO_NO_PULL,
    GPIO_PULL_UP,
    GPIO_PULL_DOWN
} GPIOPull_t;

// Interrupt mode
typedef enum {
    GPIO_INT_DISABLE,
    GPIO_INT_RISING,
    GPIO_INT_FALLING,
    GPIO_INT_BOTH,
    GPIO_INT_LOW_LEVEL,
    GPIO_INT_HIGH_LEVEL
} GPIOIntMode_t;

// GPIO configuration struct
typedef struct {
    uint32_t portBase;    // Base address (e.g. GPIO_PORTF_BASE)
    uint8_t  pin;         // Pin number (0-7)
    GPIODir_t direction;
    GPIOPull_t pull;
    bool openDrain;
    bool analog;
    bool altFunc;
    uint8_t altFuncNum;   // 0–15, for GPIOPCTL
    GPIOIntMode_t intMode;
} GPIOConfig_t;

// Interrupt Port Callback configuration struct
typedef struct {
    uint32_t portBase;
    GPIOHandler_t pinHandlers[8];
    uint8_t active;
} PortCallback_t;

extern PortCallback_t ports[6];

// API
void GPIO_Init(const GPIOConfig_t *cfg);
void GPIO_Write(uint32_t portBase, uint8_t pin, bool value);
bool GPIO_Read(uint32_t portBase, uint8_t pin);
void GPIO_Toggle(uint32_t portBase, uint8_t pin);
void GPIO_AttachISR(const GPIOConfig_t *cfg, GPIOHandler_t handler);
void GPIO_DisableInterrupt(uint32_t portBase, uint8_t pin);
void GPIO_EnableInterrupt(uint32_t portBase, uint8_t pin);
void GPIO_ClearInterrupt(uint32_t portBase, uint8_t pin);

#define GPIO_PORT_ISR(portIdx, vector) \
void vector(void) { \
    uint32_t status = HWREG(ports[portIdx].portBase + GPIO_O_MIS); \
    for (int pin = 0; pin < 8; pin++) { \
        if (status & (1 << pin)) { \
            HWREG(ports[portIdx].portBase + GPIO_O_ICR) = (1 << pin); \
            if (ports[portIdx].pinHandlers[pin]) \
                ports[portIdx].pinHandlers[pin](); \
        } \
    } \
}

#endif // GPIO_HAL_H
