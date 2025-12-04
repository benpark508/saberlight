#include "lightstrip.h"
#include "music.h"      
#include "../inc/DMATimerWrite.h"
#include "../inc/ST7735.h"
// ------------------ OUTPUT FUNCTIONS -------------------

void Out_Start(void){
	Sound_ImperialMarch();
	// LCD display opening screen
	for(int i=0; i<180; i++){ // Run for ~3 seconds (60 * 3 = 180)
        LED_Start(); 
        Lightstrip_Reset();
				Delay(500000);
				while(DMA_Status() != IDLE){};
	}
}

void Out_Block(void){
	Sound_Block();
	for(int i=0; i<20; i++){ // Run for 1/3 of a second
        LED_Block(); 
        Lightstrip_Reset();
				Delay(1000000);
				while(DMA_Status() != IDLE){};
	}
}

// Triggered by this saber hitting other player
void Out_Hit(void){
	Sound_Clash();
	// Show 
	for(int i=0; i<5; i++){	// Quick flash
        LED_Hit();
        Lightstrip_Reset();
				Delay(1000000);
				while(DMA_Status() != IDLE){};
	}
}

void Out_Win(void){
	Sound_Victory();
	for(int i=0; i<120; i++){ // Run for ~2 seconds
        LED_Win();
        Lightstrip_Reset();
				Delay(500000);
				while(DMA_Status() != IDLE){};
	}
}

void Out_Lose(void){
	Sound_Lose();
	for(int i=0; i<120; i++){ // Run for ~2 seconds
        LED_Lose();
        Lightstrip_Reset();
				Delay(500000);
				while(DMA_Status() != IDLE){};
	}
}

void Out_Off(void){
	Sound_SaberOff();
	for(int i=0; i<180; i++){ // Run for ~3 seconds
        LED_Off();
        Lightstrip_Reset();
				Delay(500000);
				while(DMA_Status() != IDLE){};
	}
}

// Triggered by other saber
void Out_Damaged(void){
	Sound_Damage();
	for(int i=0; i<5; i++){ 
        LED_Damaged();
        Lightstrip_Reset();
				while(DMA_Status() != IDLE){};
				Delay(1000000);
	}
}
