#ifndef GAME_H
#define GAME_H

#include <stdint.h>

// --- GAME CONFIGURATION ---
#define PLAYER 1  // Change to 2 for the second board/player
#define MAX_LIVES 10

// --- STATE DEFINITIONS ---
typedef enum {
    GAME_START,   // Boot up, play intro sound
    GAME_FIGHT,   // Neutral state: Detecting Swings or Incoming Hits
    GAME_BLOCK,   // Player is holding the touch sensor
    GAME_WIN,     // You won
    GAME_LOSE,    // You lost
    GAME_OVER     // Game finished, waiting for reset
} GameState_t;

// --- PUBLIC FUNCTIONS ---

// Initialize lives, pointers, and LCD text
void Game_Init(void); 

// Run one iteration of the state machine (call inside while(1))
void Game_Run(void);

// Getter for current state (optional, for main debugging)
GameState_t Game_GetState(void);

#endif