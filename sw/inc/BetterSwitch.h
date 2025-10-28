#ifndef BETTER_SWITCH_H
#define BETTER_SWITCH_H

#include <stdint.h>
#include <stdbool.h>
#include "inc/GPIO_HAL.h"

extern volatile uint32_t ticks_ms;

// API
void BetterSwitch_Init();

#endif // BETTER_SWITCH_H
