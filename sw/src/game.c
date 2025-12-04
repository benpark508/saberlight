// Game.c
// Game Engine logic for Light Saber Duel
// Integrated with Non-Blocking LED and Interrupt-Driven UART
// Runs on TM4C123
// Fall 2025

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/ST7735.h"
#include "UART4.h"
#include "game.h"
#include "music.h"
#include "lightstrip.h"
#include "microSD.h"
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
// Track the previous touch state to handle rising edges
// 0 = Idle, 1 = Hitting
static uint8_t last_touch_state = 0;

// --- HELPER FUNCTIONS ---

// Draw the UI (Lives count)
void Draw_UI(void)
{
    ST7735_SetCursor(0, 0);
    ST7735_OutString("P1 (Me): ");
    ST7735_OutUDec(MyLives);
    ST7735_OutString("  ");

    ST7735_SetCursor(0, 1);
    ST7735_OutString("P2 (Eny): ");
    ST7735_OutUDec(EnemyLives);
    ST7735_OutString("  \n");
}

// --- MAIN INIT ---
void Game_Init(void)
{
    UART4_Flush();
    MyLives = MAX_LIVES;
    EnemyLives = MAX_LIVES;
    CurrentState = GAME_START;

    ST7735_FillScreen(ST7735_BLACK);
    Draw_UI();

    // Set initial LED state
    Lightstrip_SetAnimation(ANIM_OFF);
}

// --- MAIN LOOP ---
// Assumed to be called every 1ms by the main while(1) loop
void Game_Run(void)
{

    // --- TIMING VARIABLES ---
    static uint32_t lcd_timer = 0;
    static uint32_t touch_timer = 0;
    static uint32_t led_timer = 0;

    // ----------------------------------------
    // 1. LIGHTSTRIP SCHEDULER (Every ~20ms)
    // ----------------------------------------
    led_timer++;
    if (led_timer > 20)
    {
        led_timer = 0;
        Lightstrip_Update();
    }

    // ----------------------------------------
    // 2. SENSOR INPUT & STATE LOGIC (Every ~50ms)
    // ----------------------------------------
    touch_timer++;

    if (touch_timer > 50)
    {
        touch_timer = 0;

        // --- A. READ SENSORS ---

        // Update IMU Data so DetectSwing has fresh values
        raw_imu imu_raw;
        MPU6500_getData(&imu_raw, &imu_proc);

        // Read Touch Sensors
        CAP1208_ReadCounts(touch_counts);
        int touch_sum = touch_counts[0] + touch_counts[1];
        uint8_t current_touch_state = 0;

        // --- B. DETERMINE TOUCH STATE ---
        // Simplified: Either we hit (>= 100) or we didn't.
        if (touch_sum >= 100)
        {
            current_touch_state = 1; // Hitting Enemy
        }
        else
        {
            current_touch_state = 0; // Idle
        }

        // --- C. PROCESS HITS (Only if game is active) ---
        if (CurrentState == GAME_FIGHT)
        {

            // IMPACT / HITTING ENEMY
            if (current_touch_state == 1)
            {
                // Rising edge detection (only trigger once per hit)
                if (last_touch_state == 0)
                {
                    UART4_OutChar('H');                // Send 'h' to tell enemy they were hit
                    Sound_Clash();                     // Play impact sound locally
                    Lightstrip_SetAnimation(ANIM_HIT); // Visual flash

                    // --- NEW WIN LOGIC ---
                    // Decrement Enemy lives locally because we know we hit them
                    EnemyLives--;

                    if (EnemyLives <= 0)
                    {
                        LCD_Win();
                        CurrentState = GAME_WIN;
                        Lightstrip_SetAnimation(ANIM_WIN);
                    }
                }
            }
        }

        // Update history
        last_touch_state = current_touch_state;

        // --- D. SWING DETECTION ---
        // Only detect swings in FIGHT mode
        if (CurrentState == GAME_FIGHT && current_touch_state == 0)
        {
            if (MPU6500_DetectSwing(&imu_proc))
            {
                if (!Music_IsPlaying())
                {
                    Sound_Swing();
                    // NO UART SENT HERE (per requirements)
                }
            }
        }
    }

    // ----------------------------------------
    // 3. NETWORK HANDLER (Incoming Packets)
    // ----------------------------------------
    char incoming;

    while (UART4_GetPacket(&incoming))
    {

        if (incoming == 'h')
        { // 'h' = Opponent says they hit me

            // If they hit me, I take damage.
            if (CurrentState == GAME_FIGHT || CurrentState == GAME_START)
            {
                // --- TOOK DAMAGE ---
                Sound_Damage();
                Lightstrip_SetAnimation(ANIM_DAMAGED); // Red Flash/Flicker

                MyLives--;
                LCD_ShowLives(1, MyLives);
                if (MyLives <= 0)
                {
                    // Removed sending 'x'. Enemy tracks their own win condition now.
                    LCD_Lose();
                    CurrentState = GAME_LOSE;
                    Lightstrip_SetAnimation(ANIM_LOSE);
                }
            }
        }
        // Removed checking for 'x'. We rely on local EnemyLives count (see section 2.C above).
    }

    // ----------------------------------------
    // 4. GAME STATE HOUSEKEEPING
    // ----------------------------------------
    // Handle non-combat states
    if (CurrentState == GAME_START)
    {
        LCD_Start();
        Sound_ImperialMarch();
        CurrentState = GAME_FIGHT;
        Lightstrip_SetAnimation(ANIM_IDLE);
        SD_Init();
        LCD_DrawBMP("p110.bmp", 0, 0);
    }

    // ----------------------------------------
    // 5. LCD UPDATE (Every ~100ms)
    // ----------------------------------------
    lcd_timer++;
    if (lcd_timer > 100)
    {
        lcd_timer = 0;

        if (CurrentState == GAME_FIGHT)
        {
            /*Draw_UI();
            ST7735_SetTextColor(ST7735_GREEN);
            ST7735_SetCursor(5, 5);
            ST7735_OutString("FIGHT   ");*/
        }
        else if (CurrentState == GAME_WIN)
        {
            /*ST7735_FillScreen(ST7735_GREEN);
            ST7735_SetCursor(5, 5);
            ST7735_SetTextColor(ST7735_BLACK);
            ST7735_OutString("VICTORY!");*/
            lcd_timer = 0; // stop updating
        }
        else if (CurrentState == GAME_LOSE)
        {
            /*ST7735_FillScreen(ST7735_RED);
            ST7735_SetCursor(5, 5);
            ST7735_SetTextColor(ST7735_BLACK);
            ST7735_OutString("DEFEAT  ");*/
            lcd_timer = 0; // stop updating
        }
    }
}

GameState_t Game_GetState(void)
{
    return CurrentState;
}