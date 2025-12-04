// lightstrip.h
// Header for Non-Blocking LED Driver
// Fall 2025

#ifndef LIGHTSTRIP_H
#define LIGHTSTRIP_H

#include <stdint.h>

// --- Animation States ---
// These match the logic in Game.c
enum AnimState {
    ANIM_OFF,      // Retracts blade
    ANIM_IDLE,     // Moving/Humming blade
    ANIM_BLOCK,    // Clash/Pulse effect
    ANIM_HIT,      // Flash White (Transient)
    ANIM_WIN,      // Rainbow
    ANIM_LOSE,     // Chaos/Glitch
    ANIM_DAMAGED   // Red Flicker (Transient)
};

// --- Global Color Config ---
extern int Blade_R;
extern int Blade_G;
extern int Blade_B;

// --- Prototypes ---
void Lightstrip_Init(void);
void Lightstrip_SetAnimation(int type);
void Lightstrip_Update(void); // Call this every 20ms in Game loop

#endif