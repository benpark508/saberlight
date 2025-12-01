// File **********music.c***********
// Programs to play pre-programmed music and respond to switch inputs
// Spring 2025

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/SysTickInts.h"
#include "../inc/MCP4821.h"
#include "music.h"
#include "mailbox.h"
    
#define F_CPU 80000000

// A 32-element sine wave table for sound generation.
const uint16_t SineWave[32] = {
  2048, 2447, 2831, 3185, 3495, 3750, 3939, 4056,
  4095, 4056, 3939, 3750, 3495, 3185, 2831, 2447,
  2048, 1649, 1265,  911,  601,  346,  157,   40,
     0,   40,  157,  346,  601,  911, 1265, 1649
};

uint32_t SineWaveIndex; // Index that steps through the sine wave table

// SysTick reload value is calculated as: F_CPU / (NoteFreq * SineTableSize)
// E.g., for C5 (523 Hz): 80,000,000 / (523 * 32) = 4780
#define D5  4260  //
#define E5  3792
#define F5  3580
#define Fs5 3378  // F-sharp 5
#define G5  3189
#define A5  2841
#define B5  2525
#define B4  5052
#define Cs5 4510  // C-sharp 5
#define REST 0    // A period of 0 will represent a rest

// --- Tempo Definitions ---
// Defines the note lengths for the theme's tempo.
#define q   300  // Quarter note
#define h   600  // Half note
#define dh  900  // Dotted half note
#define e   150  // Eighth note
#define de  225  // Dotted eighth note
#define s   75   // Sixteenth note

// Song: "Hedwig's Theme" from Harry Potter
Note Song[] = {
  {REST, q},  {B4, q},  {E5, de}, {G5, s},
  {Fs5, q}, {E5, h},  {B5, q},
  {A5, dh}, {Fs5, h},

  {E5, de}, {G5, s},  {Fs5, q}, {D5, h},
  {F5, q},  {Cs5, dh},
  {REST, h}
};

const uint32_t SongLength = sizeof(Song) / sizeof(Note);
uint32_t NoteIndex;      // Index for the current note in the song
uint32_t NoteDuration;   // Countdown timer for the current note's duration

// --- State Machine ---
enum PlayerState { STOPPED, PLAYING, PAUSED };
volatile enum PlayerState CurrentState;

// Forward declaration for Timer0A handler
void Timer0A_Handler(void);

//-------------- Music_Init ----------------
void Music_Init(void){
    DAC_Init(0); 
    CurrentState = STOPPED;
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

//-------------- SysTick_Handler ----------------
// High-frequency interrupt for sound wave generation.
void SysTick_Handler(void){
    if(CurrentState == PLAYING) {
        SineWaveIndex = (SineWaveIndex + 1) & 0x1F; // 0 to 31
        if(Song[NoteIndex].period == REST) {
            DAC_Out(0); // Output silence for a rest
        } else {
            DAC_Out(SineWave[SineWaveIndex]);
        }
    }
}

//-------------- Timer0A_Handler ----------------
// Slower interrupt for managing note durations.
void Timer0A_Handler(void){
    TIMER0_ICR_R = TIMER_ICR_TATOCINT; // acknowledge timer0A timeout
    if(CurrentState == PLAYING) {
        if(NoteDuration > 0) {
            NoteDuration--;
        } else {
            // Advance to the next note
            NoteIndex++;
            if(NoteIndex >= SongLength) {
                NoteIndex = 0; // Loop the song
            }
            // Load new note's period and duration
            NoteDuration = Song[NoteIndex].duration;
            if(Song[NoteIndex].period == REST) {
                NVIC_ST_CTRL_R = 0; // Disable SysTick for rests
            } else {
                NVIC_ST_RELOAD_R = Song[NoteIndex].period - 1;
                NVIC_ST_CTRL_R = 0x07; // Enable SysTick with core clock and interrupts
            }
        }
    }
}

// --- Public Control Functions ---

void Music_Play(void){
    if(CurrentState == STOPPED) {
        NoteIndex = 0;
        NoteDuration = Song[0].duration;
        if(Song[0].period != REST) {
            NVIC_ST_RELOAD_R = Song[0].period - 1;
        }
    }
    CurrentState = PLAYING;
    TIMER0_CTL_R = 0x01; // Enable Timer0A to start sequencing
    if(Song[NoteIndex].period != REST) {
        NVIC_ST_CTRL_R = 0x07; // Enable SysTick to start sound
    }
}

void Music_Pause(void){
    CurrentState = PAUSED;
    TIMER0_CTL_R = 0; // Disable Timer0A
    NVIC_ST_CTRL_R = 0; // Disable SysTick
    DAC_Out(0);
}

void Music_Stop(void){
    CurrentState = STOPPED;
    TIMER0_CTL_R = 0; // Disable Timer0A
    NVIC_ST_CTRL_R = 0; // Disable SysTick
    DAC_Out(0);
    NoteIndex = 0;
}
