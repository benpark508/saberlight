//  music.c
//  Programs to play pre-programmed music and respond to switch inputs
//  Fall 2025

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/SysTickInts.h"
#include "../inc/MCP4821.h"
#include "music.h"
#include "mailbox.h"
    
#define F_CPU 80000000


//  A 32-element sine wave table for sound generation.
const uint16_t SineWave[32] = {
  2048, 2447, 2831, 3185, 3495, 3750, 3939, 4056,
  4095, 4056, 3939, 3750, 3495, 3185, 2831, 2447,
  2048, 1649, 1265,  911,  601,  346,  157,   40,
     0,   40,  157,  346,  601,  911, 1265, 1649
};


/*
// A 32-element sine wave table at ~10% amplitude (Quiet)
const uint16_t SineWave[32] = {
  2048, 2088, 2126, 2162, 2193, 2218, 2237, 2249,
  2253, 2249, 2237, 2218, 2193, 2162, 2126, 2088,
  2048, 2008, 1970, 1934, 1903, 1878, 1859, 1847,
  1843, 1847, 1859, 1878, 1903, 1934, 1970, 2008
};
*/

uint32_t SineWaveIndex; //  Index that steps through the sine wave table

// ==== NOTE VALUES ====
#define A5  	2841
#define G3   	12755
#define C3   	19230
#define Eb4  	8035
#define Bb4  	5362
#define G4   	6377
#define D5   	4256
#define Eb5  	4017
#define E5  	3792
#define F5  	3580 
#define G5  	3189
#define Gb4  	6756
#define C5   	4780
#define C6   	2390
#define G2   	25510 
#define REST 	0      

// ==== RYTHM DEFINITIONS ====
#define q   300  // Quarter
#define h   600  // Half
#define dh  900	 // dotted half
#define e   150  // Eighth
#define de  225	 // dotted eigth
#define s   75   // Sixteenth
#define st 45
#define t   30   // Thirty-second


// ==== ALL SOUNDS ====

Note SwingArray[] = {
    {56000, 15}, {48000, 15},  // Deep rumble start
    {52000, 15}, {44000, 15},
    {48000, 15}, {40000, 15},
    {44000, 15}, {36000, 15},  // Pitch rising
    {40000, 15}, {32000, 15},
    {36000, 15}, {28000, 15},  // Peak "Whoosh" (Still low)
    {32000, 15}, {40000, 15},
    {36000, 15}, {44000, 15},  // Pitch falling
    {40000, 15}, {48000, 15},
    {44000, 15}, {52000, 15},
    {48000, 15}, {56000, 15},  // Back to deep rumble
    {REST, 0}
};

//  Song: "Imperial March"
Note ThemeArray[] = {
  {G4, q}, {G4, q}, {G4, q}, 
  {Eb4, 225}, {Bb4, 75}, {G4, q}, 
  {Eb4, 225}, {Bb4, 75}, {G4, h},
  
  {D5, q}, {D5, q}, {D5, q}, 
  {Eb5, 225}, {Bb4, 75}, {Gb4, q}, // Note: Gb4 is F#4 (harmonic shift)
  {Eb4, 225}, {Bb4, 75}, {G4, h},
  {REST, h}
};

// EFFECT: "Retraction" (Power Off)
Note SaberOffArray[] = {
    {C6, t}, {G4, t}, {C5, t}, {G4, t}, {C3, t}, {G3, t}, {G2, h}, {REST, 0}
};

// EFFECT: "Clash"
Note ClashArray[] = {
    {C6, t}, {Eb5, t}, {C6, t}, {REST, 0}
};

// EFFECT: "Block"
Note BlockArray[] = {
    {1800, 15}, {2300, 15}, {1900, 15}, {2400, 15}, 
    {2000, t}, {2200, t}, {2100, t}, {2300, t},
    {2050, t}, {2250, t}, {2150, t}, {2390, t},
    {2390, t}, {REST, 0}
};

// EFFECT: "Hum"
Note HumArray[] = {
    {G2, 100}, {REST, 0}
};

// EFFECT: "Victory"
Note VictoryArray[] = {
    {C5, s}, {C5, s}, {C5, s}, {C5, q}, 
    {Eb5, q}, {Bb4, q}, 
    {C5, s}, {Eb5, s}, {G5, s},
    {C6, h}, 
    {REST, 0}
};

// EFFECT: "Lose"
Note LoseArray[] = {
    {F5, de}, {E5, de}, {Eb5, de}, {D5, h}, 
    {REST, 0}
};

// EFFECT: "Damage" (Short Circuit Stutter)
// Three quick beeps with silence in between to sound like a glitch.
Note DamageArray[] = {
    {A5, 15}, {REST, 15}, 
    {F5, 15}, {REST, 15}, 
    {A5, 15}, {REST, 0}
};


//  ============ State Variables ============
enum PlayerState { STOPPED, PLAYING, PAUSED };
volatile enum PlayerState CurrState;

static Note *CurrentSongPtr;    // Pointer to the active song array
static uint32_t CurrentSongLen; // Length of the active song
static uint32_t NoteIndex;      // Current note index
static uint32_t NoteDuration;   // Timer countdown

//  Forward declaration for Timer0A handler
void Timer0A_Handler(void);

//  ============ Play_Internal ============
//  Sets up the engine to play a specific array.
static void Play_Internal(Note *song, uint32_t len){
    Music_Stop(); // Reset everything first to avoid glitches
    
    CurrentSongPtr = song;
    CurrentSongLen = len;
    
    NoteIndex = 0;
    CurrState = PLAYING;
    
    // Load the first note immediately
    NoteDuration = CurrentSongPtr[0].duration;
    
    // Enable Timer0A for sequencing
    TIMER0_CTL_R = 0x01; 
    
    // Enable SysTick for sound (if not a rest)
    if(CurrentSongPtr[0].period != REST) {
        NVIC_ST_RELOAD_R = CurrentSongPtr[0].period - 1;
        NVIC_ST_CTRL_R = 0x07; 
    } else {
        NVIC_ST_CTRL_R = 0; // Silence
        DAC_Out(0);
    }
}

//  ============ Music_Init ============
void Music_Init(void){
    DAC_Init(0); 
    CurrState = STOPPED;
    NoteIndex = 0;
    NoteDuration = 0;
    SineWaveIndex = 0;

    // Configure Timer0A for note sequencing (e.g., at 1000 Hz for 1ms tempo resolution)
    SYSCTL_RCGCTIMER_R |= 0x01; // 1) activate Timer0
    TIMER0_CTL_R = 0;          // 2) disable timer0A during setup
    TIMER0_CFG_R = 0;          // 3) configure for 32-bit mode
    TIMER0_TAMR_R = 0x02;      // 4) configure for periodic mode
    TIMER0_TAILR_R = (F_CPU / 1000) - 1; // 5) reload value for 1kHz
    TIMER0_TAPR_R = 0;         // 6) bus clock resolution
    TIMER0_ICR_R = 0x01;       // 7) clear timer0A timeout flag
    TIMER0_IMR_R = 0x01;       // 8) arm timeout interrupt
    NVIC_PRI4_R = (NVIC_PRI4_R & 0x00FFFFFF) | 0x40000000; // 9) priority 2
    NVIC_EN0_R = 1 << 19;      // 10) enable IRQ 19 in NVIC
    // Timer0 is initially disabled and will be enabled by Music_Play()
}

//  ============ SysTick_Handler ============
//  High-frequency interrupt for sound wave generation.
void SysTick_Handler(void){
    if(CurrState == PLAYING) {
        SineWaveIndex = (SineWaveIndex + 1) & 0x1F; // 0 to 31
        if(CurrentSongPtr[NoteIndex].period == REST) {
            DAC_Out(0); 
        } else {
            DAC_Out(SineWave[SineWaveIndex]);
        }
    }
}

//  ============ Timer0A_Handler ============
//  Slower interrupt for managing note durations.
void Timer0A_Handler(void){
    TIMER0_ICR_R = TIMER_ICR_TATOCINT; // acknowledge timer0A timeout
    
    if(CurrState == PLAYING) {
        if(NoteDuration > 0) {
            NoteDuration--;
        } else {
            // Advance to the next note
            NoteIndex++;
            
            // Check if we reached the end of the CURRENT song
            if(NoteIndex >= CurrentSongLen) {
                Music_Stop(); 
            } 
            else {
                // Load new note details from the Pointer
                NoteDuration = CurrentSongPtr[NoteIndex].duration;
                
                if(CurrentSongPtr[NoteIndex].period == REST) {
                    NVIC_ST_CTRL_R = 0; // Disable SysTick for rests
                    DAC_Out(0);
                } else {
                    NVIC_ST_RELOAD_R = CurrentSongPtr[NoteIndex].period - 1;
                    NVIC_ST_CTRL_R = 0x07; // Enable SysTick
                }
            }
        }
    }
}

//  ============ Public Control Functions ============

void Sound_ImperialMarch(void){
    Play_Internal(ThemeArray, sizeof(ThemeArray)/sizeof(Note));
}

void Sound_SaberOff(void){
    Play_Internal(SaberOffArray, sizeof(SaberOffArray)/sizeof(Note));
}

void Sound_Clash(void){
    Play_Internal(ClashArray, sizeof(ClashArray)/sizeof(Note));
}

void Sound_Block(void){
    Play_Internal(BlockArray, sizeof(BlockArray)/sizeof(Note));
}

void Sound_Hum(void){
    Play_Internal(HumArray, sizeof(HumArray)/sizeof(Note));
}

void Sound_Victory(void){
    Play_Internal(VictoryArray, sizeof(VictoryArray)/sizeof(Note));
}

void Sound_Lose(void){
    Play_Internal(LoseArray, sizeof(LoseArray)/sizeof(Note));
}

void Sound_Damage(void){
    Play_Internal(DamageArray, sizeof(DamageArray)/sizeof(Note));
}

void Sound_Swing(void){
    Play_Internal(SwingArray, sizeof(SwingArray)/sizeof(Note));
}

uint8_t Music_IsPlaying(void){
    return (CurrState == PLAYING);
}

void Music_Pause(void){
    CurrState = PAUSED;
    TIMER0_CTL_R = 0; // Disable Timer0A
    NVIC_ST_CTRL_R = 0; // Disable SysTick
    DAC_Out(0);
}

void Music_Stop(void){
    CurrState = STOPPED;
    TIMER0_CTL_R = 0; // Disable Timer0A
    NVIC_ST_CTRL_R = 0; // Disable SysTick
    DAC_Out(0);
    NoteIndex = 0;
}