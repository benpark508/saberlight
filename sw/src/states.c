#include <stdint.h>
#include "lightstrip.h"
#include "music.h"      
#include "UART4.h" 
#include "../inc/DMATimerWrite.h"
#include "../inc/ST7735.h"

// ------------------ HELPER FUNCTIONS -------------------

// Standard delay function for TM4C123
// ulCount of 533,333 ~= 20ms at 80MHz clock
// This matches the "Call every ~20ms" requirement of Lightstrip_Update
void Delay(unsigned long ulCount){
  __asm (  "subs    r0, #1\n"
           "bne     Delay\n"
           "bx      lr\n");
}

// ------------------ OUTPUT FUNCTIONS -------------------

void Out_Start(void){
    Sound_ImperialMarch();
    
    // Set to IDLE (Moving/Humming Blade)
    Lightstrip_SetAnimation(ANIM_IDLE);

    // Run for ~3 seconds (150 frames * 20ms = 3s)
    for(int i=0; i<150; i++){ 
        Lightstrip_Update(); // Render frame & trigger DMA
        Delay(533333);       // Wait ~20ms
    }
}

void Out_Block(void){
    Sound_Block();
    
    // Set to Clash/Block effect
    Lightstrip_SetAnimation(ANIM_BLOCK);
    
    // Run for ~0.4 seconds (20 frames)
    for(int i=0; i<20; i++){ 
        Lightstrip_Update();
        Delay(533333); 
    }
    
    // Return to Idle state after block is done
    Lightstrip_SetAnimation(ANIM_IDLE);
    Lightstrip_Update();
}

// Triggered by this saber hitting other player
void Out_Hit(void){
    Sound_Clash();
    
    // Set to Flash effect (Transient)
    // The driver auto-reverts this to IDLE after 5 frames,
    // but we loop here to ensure the visual completes before returning to game logic.
    Lightstrip_SetAnimation(ANIM_HIT);
    
    for(int i=0; i<10; i++){ // Run for 10 frames (0.2s) to ensure effect finishes
        Lightstrip_Update();
        Delay(533333);
    }
}

void Out_Win(void){
    Sound_Victory();
    
    // Set to Rainbow effect
    Lightstrip_SetAnimation(ANIM_WIN);
    
    // Run for ~2 seconds
    for(int i=0; i<100; i++){ 
        Lightstrip_Update();
        Delay(533333);
    }
}

void Out_Lose(void){
    Sound_Lose();
    
    // Set to Chaos/Glitch effect
    Lightstrip_SetAnimation(ANIM_LOSE);
    
    // Run for ~2 seconds
    for(int i=0; i<100; i++){ 
        Lightstrip_Update();
        Delay(533333);
    }
}

void Out_Off(void){
    Sound_SaberOff();
    
    // Set to Retract effect
    Lightstrip_SetAnimation(ANIM_OFF);
    
    // Run for ~3 seconds to allow full retraction
    for(int i=0; i<150; i++){ 
        Lightstrip_Update();
        Delay(533333);
    }
}

// Triggered by other saber hitting us
void Out_Damaged(void){
    Sound_Damage();
    
    // Set to Flicker effect (Transient)
    // Driver auto-reverts after 10 frames
    Lightstrip_SetAnimation(ANIM_DAMAGED);
    
    for(int i=0; i<15; i++){ // Run slightly longer than the 10-frame internal counter
        Lightstrip_Update();
        Delay(533333);
    }
}