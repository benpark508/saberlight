// mpr121.c
// Runs on TM4C123
// Driver for MPR121 capacitive touch sensor
// Ben Park
// October 28 2025

#include <stdint.h>
#include "../inc/mpr121.h"

static void MPR121_WriteReg(uint8_t reg, uint8_t value) {
    uint8_t data[2] = { reg, value };
    I2C1_Write_Buffer(MPR121_ADDRESS, 2, data);
}

static uint8_t MPR121_ReadReg8(uint8_t reg) {
    uint8_t data = reg;
    I2C1_Write_Byte(MPR121_ADDRESS, data);
    I2C1_Read_Byte(MPR121_ADDRESS, &data);
    return data;
}

static uint16_t MPR121_ReadReg16(uint8_t reg) {
    uint8_t data[2];
    I2C1_Write_Byte(MPR121_ADDRESS, reg);
    I2C1_Read_Buffer(MPR121_ADDRESS, 2, data);
    return ((uint16_t)data[0] | (uint16_t)(data[1] << 8));
}

// === Soft Reset and Basic Config ===
void MPR121_Init(void) {

    // 1. Soft reset
    MPR121_WriteReg(MPR121_SOFT_RESET, 0x63);

    // 2. Stop mode (electrodes disabled)
    MPR121_WriteReg(MPR121_ELECTRODE_CONFIG, 0x00);

    // 4. Set touch/release thresholds (all electrodes)
    MPR121_SetThresholds(15, 7);

    // 5. Configure filtering and debounce
    MPR121_WriteReg(MPR121_DEBOUNCE, 0x00);   // No debounce
    MPR121_WriteReg(MPR121_CONFIG1, 0x10);    // FFI=0, CDC=16uA
    MPR121_WriteReg(MPR121_CONFIG2, 0x20);    // CDT=1, SFI=0, ESI=1ms

    // 6. Enable all 12 electrodes (enter run mode)
    MPR121_WriteReg(MPR121_ELECTRODE_CONFIG, 0x8F); // CL=10, ELE_EN=15 (12 electrodes active)
}

// === Read 12-bit touch status ===
uint16_t MPR121_ReadTouchStatus(void) {
    return MPR121_ReadReg16(MPR121_TOUCH_STATUS_L);
}

// === Set electrode configuration (enable/disable electrodes) ===
int MPR121_SetElectrodeConfig(uint8_t config) {
    if (config > 0x8F) return -1; // invalid config
    MPR121_WriteReg(MPR121_ELECTRODE_CONFIG, config);
    return 0;
}

// === Set thresholds for all 12 electrodes ===
int MPR121_SetThresholds(uint8_t touchThreshold, uint8_t releaseThreshold) {
    if (touchThreshold > 255 || releaseThreshold > 255) return -1;

    // Enter stop mode to modify thresholds
    uint8_t prev_config = MPR121_ReadReg8(MPR121_ELECTRODE_CONFIG);
    if (prev_config != 0) {
        MPR121_WriteReg(MPR121_ELECTRODE_CONFIG, 0x00);
    }

    // Write thresholds for all 12 electrodes
    for (uint8_t i = 0; i < 12; i++) {
        MPR121_WriteReg(MPR121_TOUCH_THRESHOLD_BASE + i * 2, touchThreshold);
        MPR121_WriteReg(MPR121_RELEASE_THRESHOLD_BASE + i * 2, releaseThreshold);
    }

    // Restore previous config (re-enable run mode)
    if (prev_config != 0) {
        MPR121_WriteReg(MPR121_ELECTRODE_CONFIG, prev_config);
    }

    return 0;
}
