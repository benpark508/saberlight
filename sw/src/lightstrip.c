// lightstrip.c
// Non-Blocking WS2812 Driver for TM4C123
// Adapted for Light Saber Game Engine
// Fall 2025

#include <stdint.h>
#include "../inc/PLL.h"
#include "../inc/DMATimerWrite.h"
#include "../inc/tm4c123gh6pm.h"
#include "lightstrip.h"

// --- Hardware Definitions ---
#define GPIO_PORTE0 (*((volatile uint32_t *)0x40024004))
#define NUMLEDS   30       
#define SIZE      3*8*4*NUMLEDS 
#define PINGPONGS 9       

// --- Buffers & State ---
uint8_t Buffer[SIZE];      
uint32_t Cursor;           
int animation_phase = 0;   
int rainbow_phase = 0;
static int current_anim_state = ANIM_OFF;
static int anim_frame_counter = 0; // Tracks how long an effect has run

// --- Global Colors ---
int Blade_R = 0;
int Blade_G = 255; // Default Green
int Blade_B = 0;

// --- Internal Helper Prototypes ---
void sendreset(void);
void clearbuffer(void);
void addcolor(uint8_t red, uint8_t green, uint8_t blue);
void Lightstrip_Show(void); // Triggers DMA

// --- Effect Prototypes ---
void LED_Moving(void);  // Idle Hum
void LED_Block(void);   // Clash
void LED_Hit(void);     // Flash
void LED_Win(void);     // Rainbow
void LED_Lose(void);    // Glitch
void LED_Off(void);     // Retract
void LED_Damaged(void); // Flicker

// -----------------------------------------------------------------------
//                           PUBLIC FUNCTIONS
// -----------------------------------------------------------------------

void Lightstrip_Init(void){
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R4; // Port E
    while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R4) == 0){};
    
    GPIO_PORTE_DIR_R |= 0x01;    // PE0 Output
    GPIO_PORTE_AFSEL_R &= ~0x01; 
    GPIO_PORTE_DR8R_R |= 0x01;   
    GPIO_PORTE_DEN_R |= 0x01;    
    GPIO_PORTE_PCTL_R &= ~0x0000000F;
    GPIO_PORTE_AMSEL_R &= ~0x01; 
    
    sendreset();
    DMA_Init(32, (uint32_t *)&GPIO_PORTE0); 
    
    clearbuffer();
    Lightstrip_Show();
}

// Set the current animation state
// Resets the frame counter if the state changes or if re-triggering a transient effect
void Lightstrip_SetAnimation(int type){
    if (current_anim_state != type || type == ANIM_HIT || type == ANIM_DAMAGED) {
        current_anim_state = type;
        anim_frame_counter = 0;
    }
}

// THE ENGINE: Call this every ~20ms from Game.c
void Lightstrip_Update(void){
    
    // 1. Safety Check: Don't touch buffer if DMA is still sending previous frame
    if(DMA_Status() != IDLE) return;

    // 2. Prepare Buffer
    clearbuffer();

    // 3. Render Frame based on State
    switch(current_anim_state){
        case ANIM_IDLE:
            LED_Moving(); 
            break;
            
        case ANIM_BLOCK:
            LED_Block();
            break;
            
        case ANIM_HIT:
            if(anim_frame_counter < 5){ // Flash for 5 frames (~100ms)
                LED_Hit();
                anim_frame_counter++;
            } else {
                current_anim_state = ANIM_IDLE; // Auto-revert
            }
            break;
            
        case ANIM_DAMAGED:
            if(anim_frame_counter < 10){ // Flicker for 10 frames (~200ms)
                LED_Damaged();
                anim_frame_counter++;
            } else {
                current_anim_state = ANIM_IDLE; // Auto-revert
            }
            break;
            
        case ANIM_WIN:
            LED_Win();
            break;
            
        case ANIM_LOSE:
            LED_Lose();
            break;
            
        case ANIM_OFF:
            LED_Off();
            break;
            
        default:
            LED_Off();
            break;
    }

    // 4. Send to LEDs
    Lightstrip_Show();
}

// -----------------------------------------------------------------------
//                           INTERNAL HELPERS
// -----------------------------------------------------------------------

void Delay_Short(uint32_t ulCount){
    while(ulCount--){ __asm(" NOP"); }
}

void sendreset(void){
    GPIO_PORTE0 = 0x00;           
    Delay_Short(1333); // ~50us
}

void clearbuffer(void){
    int i;
    Cursor = 0;                 
    for(i=0; i<SIZE; i=i+4){
        Buffer[i] = 1;       
        Buffer[i+1] = 0;
        Buffer[i+2] = 0;
        Buffer[i+3] = 0;
    }
}

void addcolor(uint8_t red, uint8_t green, uint8_t blue){
    int i;
    // Green
    for(i=0x80; i>0x00; i=i>>1){   
        Buffer[Cursor] = 1;
        Buffer[Cursor+1] = (green&i) ? 1 : 0;
        Buffer[Cursor+2] = 0; Buffer[Cursor+3] = 0;
        Cursor += 4;
    }
    // Red
    for(i=0x80; i>0x00; i=i>>1){   
        Buffer[Cursor] = 1;
        Buffer[Cursor+1] = (red&i) ? 1 : 0;
        Buffer[Cursor+2] = 0; Buffer[Cursor+3] = 0;
        Cursor += 4;
    }
    // Blue
    for(i=0x80; i>0x00; i=i>>1){   
        Buffer[Cursor] = 1;
        Buffer[Cursor+1] = (blue&i) ? 1 : 0;
        Buffer[Cursor+2] = 0; Buffer[Cursor+3] = 0;
        Cursor += 4;
    }
    if(Cursor >= SIZE) Cursor = 0; 
}

void Lightstrip_Show(void){
    sendreset();    
    DMA_Transfer((uint8_t *)Buffer, SIZE/PINGPONGS, PINGPONGS);
}

// -----------------------------------------------------------------------
//                           EFFECT LOGIC
// -----------------------------------------------------------------------

// --- IDLE: Moving Blade (Hum) ---
void LED_Moving(void) {
    int i;
    animation_phase++;
    if(animation_phase > 5) animation_phase = 0;

    for(i = 0; i < NUMLEDS; i++) {
        if((i + animation_phase) % 5 == 0) {
            addcolor(Blade_R, Blade_G, Blade_B); 
        } else {
            addcolor(Blade_R/5, Blade_G/5, Blade_B/5); 
        }
    }
}

// --- BLOCK: Clash Pulse ---
void LED_Block(void) {
    int i;
    int clash_center = NUMLEDS / 2;      
    int start = clash_center - 4;
    int end   = clash_center + 4;
    
    animation_phase++; 
    int cycle = animation_phase % 10;
    int brightness_divider = (cycle < 5) ? 1 : 4;

    for(i = 0; i < NUMLEDS; i++) {
        if(i >= start && i <= end) {
            // Strobing Spark
            if ((i + animation_phase) % 3 == 0)      addcolor(50, 50, 50); // White
            else if ((i + animation_phase) % 3 == 1) addcolor(0, 0, 255);  // Yellow
            else                                     addcolor(0, 0, 0);    // Black
        } else {
            addcolor(Blade_R / brightness_divider, 
                     Blade_G / brightness_divider, 
                     Blade_B / brightness_divider);
        }
    }
}

// --- HIT: Full Flash ---
void LED_Hit(void) {
    int i;
    for(i = 0; i < NUMLEDS; i++) {
        addcolor(100, 100, 0); // Flash Bright Yellow
    }
}

// --- WIN: Rainbow ---
void LED_Win(void) {
    int i, pixel_hue, r, g, b;
    rainbow_phase = (rainbow_phase + 5) & 255; 

    for (i = 0; i < NUMLEDS; i++) {
        pixel_hue = (rainbow_phase + (i * 10)) & 255; 

        if (pixel_hue < 85) {
            r = 255 - pixel_hue * 3; g = pixel_hue * 3; b = 0;
        } else if (pixel_hue < 170) {
            pixel_hue -= 85;
            r = 0; g = 255 - pixel_hue * 3; b = pixel_hue * 3;
        } else {
            pixel_hue -= 170;
            r = pixel_hue * 3; g = 0; b = 255 - pixel_hue * 3;
        }
        addcolor(r/4, g/4, b/4);
    }
}

// --- LOSE: Chaos Glitch ---
void LED_Lose(void) {
    int i;
    animation_phase++; 

    for(i = 0; i < NUMLEDS; i++) {
        int chaos = (i * 13 + animation_phase * 23) % 100;
        if (chaos > 92)      addcolor(255, 255, 255); // Arc Flash
        else if (chaos > 75) addcolor(0, 0, 0);       // Dead Zone
        else if (chaos > 50) addcolor(Blade_R/10, Blade_G/10, Blade_B/10); // Brownout
        else                 addcolor(Blade_R/3, Blade_G/3, Blade_B/3);    // Unstable
    }
}

// --- OFF: Retract ---
// Note: We use a static variable to remember height across frames
void LED_Off(void) {
    int i;
    static int blade_height = NUMLEDS; 
    static int slow_count = 0;
    
    // Only used to reset height when we first enter this state
    if (current_anim_state != ANIM_OFF) blade_height = NUMLEDS; 

    slow_count++;
    if (slow_count >= 2){ // Speed of retraction
        slow_count = 0;
        if (blade_height > 0) blade_height--;
    }

    for(i = 0; i < NUMLEDS; i++) {
        if (i < blade_height) addcolor(Blade_R, Blade_G, Blade_B); 
        else                  addcolor(0, 0, 0);
    }
}

// --- DAMAGED: Flicker ---
void LED_Damaged(void) {
    int i;
    animation_phase++;
    
    if (animation_phase % 2 == 0) {
        for(i = 0; i < NUMLEDS; i++) addcolor(0, 0, 0); 
    } else {
        for(i = 0; i < NUMLEDS; i++) addcolor(Blade_R, Blade_G, Blade_B);
    }
}