#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/ST7735.h"
#include "UART4.h"  
#include "Game.h"
#include "music.h"
#include "../inc/mpu6500.h"
#include "../inc/cap1208.h"

// --- EXTERNAL VARIABLES ---
extern processed_imu imu_proc; 

// --- GLOBAL VARIABLES ---
GameState_t CurrentState = GAME_START;
int MyLives = MAX_LIVES;
int EnemyLives = MAX_LIVES;

// Touch sensor data
int8_t touch_counts[4]; // Array for C1-C4
static uint8_t last_touch_state = 0;

// --- HELPER FUNCTIONS ---

// Draw the UI (Lives count)
void Draw_UI(void) {
    ST7735_SetCursor(0, 0);
    ST7735_OutString("P1 (Me): ");
    ST7735_OutUDec(MyLives);
    ST7735_OutString("  "); 
    
    ST7735_SetCursor(0, 1);
    ST7735_OutString("P2 (Eny): ");
    ST7735_OutUDec(EnemyLives);
    ST7735_OutString("  \n"); 
}

// Check UART4 for incoming data
char Check_Network(void) {
    if((UART4_FR_R & 0x10) == 0) {
        return (char)(UART4_DR_R & 0xFF);
    }
    return 0;
}

// --- MAIN INIT ---
void Game_Init(void) {
    MyLives = MAX_LIVES;
    EnemyLives = MAX_LIVES;
    CurrentState = GAME_START;
    
    ST7735_FillScreen(ST7735_BLACK);
    Draw_UI();
}

// --- MAIN LOOP ---
void Game_Run(void) {
    
    // Performance Throttling for LCD
    // We run Game_Run every 1ms. We only want to draw to LCD every ~100ms
    static uint32_t lcd_timer = 0;
    uint8_t update_screen = 0;
    lcd_timer++;
    if(lcd_timer > 100) {
        lcd_timer = 0;
        update_screen = 1;
    }

    // 1. READ TOUCH INPUTS (Tube Logic)
    // We only read touch every 50ms to save I2C overhead
    static uint32_t touch_timer = 0;
    touch_timer++;
    
    uint8_t current_touch_state = 0;
    
    if(touch_timer > 50) {
        touch_timer = 0;
        CAP1208_ReadCounts(touch_counts); // Reads C1-C4 into array
        
        // Tube Logic: Sum of C1 (Index 0) and C7 (Index 1) >= 100
        if(touch_counts[0] + touch_counts[1] >= 100) {
            current_touch_state = 1;
        }
    } else {
        current_touch_state = last_touch_state; // Maintain state between reads
    }
    
    // 2. CHECK NETWORK (INCOMING DAMAGE)
    char incoming = Check_Network();
    
    if (incoming == 'h') {
        if (CurrentState == GAME_BLOCK) {
            // SUCCESSFUL BLOCK!
            Sound_Clash(); 
        } 
        else if (CurrentState == GAME_FIGHT || CurrentState == GAME_START) {
            // WE TOOK DAMAGE
            Sound_Damage();
            MyLives--;
            if (MyLives <= 0) {
                UART4_OutChar('x'); // Tell enemy I died
                CurrentState = GAME_LOSE;
            }
        }
    }
    else if (incoming == 'x') {
        CurrentState = GAME_WIN;
    }

    // 3. STATE MACHINE
    switch(CurrentState) {
        
        // ============= START =============
        case GAME_START:
            if(update_screen) ST7735_OutString("Ready...");
            Sound_ImperialMarch(); 
            if(!Music_IsPlaying()){
                 CurrentState = GAME_FIGHT;
                 ST7735_FillScreen(ST7735_BLACK); 
                 Draw_UI();
            }
            break;

        // ============= FIGHT =============
        case GAME_FIGHT:
            if(update_screen) {
                Draw_UI();
                ST7735_SetTextColor(ST7735_GREEN);
                ST7735_SetCursor(5, 5);
                ST7735_OutString("FIGHT   ");
            }

            // COND 1: SWITCH TO BLOCK (Rising Edge)
            if (current_touch_state == 1 && last_touch_state == 0) { 
                Sound_Block(); 
                CurrentState = GAME_BLOCK;
            }
            // COND 1b: Sustain Block
            else if (current_touch_state == 1) {
                CurrentState = GAME_BLOCK;
            }
            
            // COND 2: ATTACK (SWING)
            // DetectSwing is fast, so we check it every cycle (1ms)
            else if (MPU6500_DetectSwing(&imu_proc)) {
                if (!Music_IsPlaying()) {
                    Sound_Swing();
                    UART4_OutChar('h'); // Send 'h' to opponent
                }
            }
            break;

        // ============= BLOCKING =============
        case GAME_BLOCK:
            if(update_screen) {
                Draw_UI();
                ST7735_SetTextColor(ST7735_BLUE);
                ST7735_SetCursor(5, 5);
                ST7735_OutString("BLOCKING");
            }
            
            // COND 1: RELEASE BLOCK
            if (current_touch_state == 0) {
                CurrentState = GAME_FIGHT;
            }
            break;

        // ============= WIN =============
        case GAME_WIN:
            if(update_screen) {
                ST7735_FillScreen(ST7735_GREEN);
                ST7735_SetCursor(5, 5);
                ST7735_SetTextColor(ST7735_BLACK);
                ST7735_OutString("VICTORY!");
            }
            Sound_Victory();
            CurrentState = GAME_OVER;
            break;

        // ============= LOSE =============
        case GAME_LOSE:
            if(update_screen) {
                ST7735_FillScreen(ST7735_RED);
                ST7735_SetCursor(5, 5);
                ST7735_SetTextColor(ST7735_BLACK);
                ST7735_OutString("DEFEAT");
            }
            Sound_Lose();
            CurrentState = GAME_OVER;
            break;

        // ============= OVER =============
        case GAME_OVER:
            break;
    }
    
    last_touch_state = current_touch_state;
}

GameState_t Game_GetState(void) {
    return CurrentState;
}