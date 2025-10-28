#include "GPIO_HAL.h"

#define PORT_A_INDEX 0
#define PORT_B_INDEX 1
#define PORT_C_INDEX 2
#define PORT_D_INDEX 3
#define PORT_E_INDEX 4
#define PORT_F_INDEX 5

PortCallback_t ports[6] = {
    {GPIO_PORTA_BASE, {0}, 0},
    {GPIO_PORTB_BASE, {0}, 0},
    {GPIO_PORTC_BASE, {0}, 0},
    {GPIO_PORTD_BASE, {0}, 0},
    {GPIO_PORTE_BASE, {0}, 0},
    {GPIO_PORTF_BASE, {0}, 0}
};

static int PortBaseToIndex(uint32_t base) {
    switch (base) {
        case GPIO_PORTA_BASE: return 0;
        case GPIO_PORTB_BASE: return 1;
        case GPIO_PORTC_BASE: return 2;
        case GPIO_PORTD_BASE: return 3;
        case GPIO_PORTE_BASE: return 4;
        case GPIO_PORTF_BASE: return 5;
        default: return -1;
    }
}

static uint32_t pinMask(uint8_t pin) {
    return (1U << pin);
}

static void GPIO_EnablePort(uint32_t portBase) {
    uint32_t bit = 0;

    switch (portBase) {
        case GPIO_PORTA_BASE: bit = SYSCTL_RCGCGPIO_R0; break;
        case GPIO_PORTB_BASE: bit = SYSCTL_RCGCGPIO_R1; break;
        case GPIO_PORTC_BASE: bit = SYSCTL_RCGCGPIO_R2; break;
        case GPIO_PORTD_BASE: bit = SYSCTL_RCGCGPIO_R3; break;
        case GPIO_PORTE_BASE: bit = SYSCTL_RCGCGPIO_R4; break;
        case GPIO_PORTF_BASE: bit = SYSCTL_RCGCGPIO_R5; break;
        default: return;
    }

    SYSCTL_RCGCGPIO_R |= bit;
    while ((SYSCTL_PRGPIO_R & bit) == 0) {}
}

void GPIO_Init(const GPIOConfig_t *cfg) {
    GPIO_EnablePort(cfg->portBase); // Enabling Clock gate on Port
    uint32_t mask = pinMask(cfg->pin);

    // Unlock special pins if needed
    if ((cfg->portBase == GPIO_PORTF_BASE && cfg->pin == 0) ||
        (cfg->portBase == GPIO_PORTD_BASE && cfg->pin == 7) ||
        (cfg->portBase == GPIO_PORTC_BASE && cfg->pin <= 3)) 
    {
        HWREG(cfg->portBase + GPIO_O_LOCK) = GPIO_LOCK_KEY;
        HWREG(cfg->portBase + GPIO_O_CR) |= (1U << cfg->pin);
    }

    // Direction
    if (cfg->direction == GPIO_DIR_OUTPUT)
        HWREG(cfg->portBase + GPIO_O_DIR) |= mask;
    else
        HWREG(cfg->portBase + GPIO_O_DIR) &= ~mask;

    // Pull-up / Pull-down
    HWREG(cfg->portBase + GPIO_O_PUR) &= ~mask;
    HWREG(cfg->portBase + GPIO_O_PDR) &= ~mask;
    if (cfg->pull == GPIO_PULL_UP) {
        HWREG(cfg->portBase + GPIO_O_PUR) |= mask;
    } else if (cfg->pull == GPIO_PULL_DOWN) {
        HWREG(cfg->portBase + GPIO_O_PDR) |= mask;
    }

    // Drive strength

    // Open-drain
    if (cfg->openDrain)
        HWREG(cfg->portBase + GPIO_O_ODR) |= mask;
    else
        HWREG(cfg->portBase + GPIO_O_ODR) &= ~mask;

    // Analog / Digital
    if (cfg->analog) {
        HWREG(cfg->portBase + GPIO_O_AMSEL) |= mask;
        HWREG(cfg->portBase + GPIO_O_DEN)   &= ~mask;
    } else {
        HWREG(cfg->portBase + GPIO_O_AMSEL) &= ~mask;
        HWREG(cfg->portBase + GPIO_O_DEN)   |= mask;
    }

    // Alternate function
    if (cfg->altFunc) {
        HWREG(cfg->portBase + GPIO_O_AFSEL) |= mask;
        HWREG(cfg->portBase + GPIO_O_PCTL) &= ~(0xF << (cfg->pin * 4));
        HWREG(cfg->portBase + GPIO_O_PCTL) |= (cfg->altFuncNum << (cfg->pin * 4));
    } else {
        HWREG(cfg->portBase + GPIO_O_AFSEL) &= ~mask;
    }

    // Interrupt config
    HWREG(cfg->portBase + GPIO_O_IM) &= ~mask;  // Disable first
    if (cfg->intMode != GPIO_INT_DISABLE) {
        // Edge or level
        if (cfg->intMode == GPIO_INT_RISING || cfg->intMode == GPIO_INT_FALLING || cfg->intMode == GPIO_INT_BOTH) {
            HWREG(cfg->portBase + GPIO_O_IS) &= ~mask; // Edge-sensitive
            if (cfg->intMode == GPIO_INT_BOTH)
                HWREG(cfg->portBase + GPIO_O_IBE) |= mask;
            else {
                HWREG(cfg->portBase + GPIO_O_IBE) &= ~mask;
                if (cfg->intMode == GPIO_INT_RISING)
                    HWREG(cfg->portBase + GPIO_O_IEV) |= mask;
                else
                    HWREG(cfg->portBase + GPIO_O_IEV) &= ~mask;
            }
        } else {
            HWREG(cfg->portBase + GPIO_O_IS) |= mask; // Level-sensitive
            if (cfg->intMode == GPIO_INT_HIGH_LEVEL)
                HWREG(cfg->portBase + GPIO_O_IEV) |= mask;
            else
                HWREG(cfg->portBase + GPIO_O_IEV) &= ~mask;
        }
        // Clear any pending
        HWREG(cfg->portBase + GPIO_O_ICR) = mask;
        // Enable interrupt
        HWREG(cfg->portBase + GPIO_O_IM) |= mask;
    }
}

// API functions
void GPIO_Write(uint32_t portBase, uint8_t pin, bool value) {
    uint32_t mask = pinMask(pin);
    if (value)
        HWREG(portBase + (GPIO_O_DATA + (mask << 2))) = mask;
    else
        HWREG(portBase + (GPIO_O_DATA + (mask << 2))) = 0;
}

bool GPIO_Read(uint32_t portBase, uint8_t pin) {
    uint32_t mask = pinMask(pin);
    return (HWREG(portBase + (GPIO_O_DATA + (mask << 2))) & mask) ? true : false;
}

void GPIO_Toggle(uint32_t portBase, uint8_t pin) {
    uint32_t mask = pinMask(pin);
    uint32_t val = HWREG(portBase + (GPIO_O_DATA + (mask << 2)));
    HWREG(portBase + (GPIO_O_DATA + (mask << 2))) = val ^ mask;
}

void GPIO_DisableInterrupt(uint32_t portBase, uint8_t pin) {
    HWREG(portBase + GPIO_O_IM) &= ~(1U << pin);
}

void GPIO_EnableInterrupt(uint32_t portBase, uint8_t pin) {
    HWREG(portBase + GPIO_O_IM) |= (1U << pin);
}

void GPIO_ClearInterrupt(uint32_t portBase, uint8_t pin) {
    HWREG(portBase + GPIO_O_ICR) = (1U << pin);
}

void GPIO_AttachISR(const GPIOConfig_t *cfg, GPIOHandler_t handler) {
    int portIdx = PortBaseToIndex(cfg->portBase);
    if (portIdx < 0 || cfg->pin > 7) return;

    ports[portIdx].pinHandlers[cfg->pin] = handler;

    // Enable NVIC if not already
    if (!ports[portIdx].active) {
        ports[portIdx].active = 1;
        switch (portIdx) {
            case PORT_A_INDEX: NVIC_EN0_R |= (1 << 0); break;
            case PORT_B_INDEX: NVIC_EN0_R |= (1 << 1); break;
            case PORT_C_INDEX: NVIC_EN0_R |= (1 << 2); break;
            case PORT_D_INDEX: NVIC_EN0_R |= (1 << 3); break;
            case PORT_E_INDEX: NVIC_EN0_R |= (1 << 4); break;
            case PORT_F_INDEX: NVIC_EN0_R |= (1 << 30); break;
        }
    }
}