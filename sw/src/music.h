// File **********music.h***********
// Lab 5
// Programs to play pre-programmed music and respond to switch
// inputs.
// EE445L Spring 2025

#include <stdint.h>

#ifndef MUSIC_H
#define MUSIC_H

// A Note is defined by its pitch (period of the SysTick timer) and its duration.
// The lab manual requires designing a data structure to hold note and duration information.
typedef const struct {
  uint32_t period; // SysTick reload value for this note's frequency
  uint32_t duration; // Duration of the note in song-tempo units
} Note;

//-------------- Music_Init ----------------
// Initializes SysTick, a periodic timer for note sequencing,
// and the DAC.
// Inputs: none
// Outputs: none
// Called once before any other music functions.
void Music_Init(void);

//-------------- Music_Play ----------------
// Starts playing the current song from the beginning.
// Inputs: none
// Outputs: none
void Music_Play(void);

//-------------- Music_Pause ----------------
// Pauses the currently playing song.
// Inputs: none
// Outputs: none
void Music_Pause(void);

//-------------- Music_Stop ----------------
// Stops the song and resets to the beginning.
// Inputs: none
// Outputs: none
void Music_Stop(void);

uint8_t Music_IsPlaying(void);

void Sound_ImperialMarch(void); // The Theme
void Sound_SaberOff(void);      // Retraction (Lose/End)
void Sound_Clash(void);         // Hit/Block
void Sound_Block(void);
void Sound_Hum(void);
void Sound_Victory(void);
void Sound_Lose(void);
void Sound_Damage(void);
void Sound_Swing(void);

#endif // MUSIC_H