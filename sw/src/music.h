// File **********music.h***********
// Lab 5
// Programs to play pre-programmed music and respond to switch
// inputs.
// EE445L Spring 2025


//-------------- Music_Init ----------------
// activate SysTick, Timer0 and Timer1 periodic interrupts,
// and DAC
// Inputs: none
// Outputs: none
// called once
void Music_Init(void);

void Play(void);

void Stop(void);

void Rewind(void);

void ChangeTempo(void);