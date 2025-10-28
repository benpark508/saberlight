// File **********music.c***********
// Programs to play pre-programmed music and respond to switch inputs
// Spring 2025

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/SysTickInts.h"
#include "../inc/TLV5616.h"
#include "music.h"
#include "mailbox.h"
#include "../inc/Timer1A.h"
#include "../inc/Timer2A.h"
// write this


typedef const struct
{
    uint32_t Pitch;    // SysTick reload value for frequency
    uint32_t Duration; // Duration in metronome ticks
} Note;

// Octave 2
#define C2 19111
#define CS2 18039
#define D2 17026
#define DS2 16071
#define E2 15174
#define F2 14317
#define FS2 13514
#define G2 12755
#define GS2 12039
#define A2 11363
#define AS2 10726
#define B2 10123

// Octave 3
#define C3 9556
#define CS3 9019
#define D3 8513
#define DS3 8035
#define E3 7584
#define F3 7159
#define FS3 6757
#define G3 6378
#define GS3 6019
#define A3 5681
#define AS3 5362
#define B3 5062

// Octave 4
#define C4 4779
#define CS4 4510
#define D4 4257
#define DS4 4018
#define E4 3792
#define F4 3579
#define FS4 3378
#define G4 3189
#define GS4 3009
#define A4 2841
#define AS4 2680
#define B4 2531

// Octave 5
#define C5 2389
#define CS5 2255
#define D5 2128
#define DS5 2009
#define E5 1896
#define F5 1790
#define FS5 1689
#define G5 1594
#define GS5 1505
#define A5 1420
#define AS5 1340
#define B5 1265

// Octave 6
#define C6 1194
#define CS6 1127
#define D6 1064
#define DS6 1004
#define E6 948
#define F6 895
#define FS6 845
#define G6 797
#define GS6 752
#define A6 710
#define AS6 670
#define B6 633

#define REST 0

uint32_t I1, I2;
// 11-bit 64-element sine wave
const uint16_t Wave[64] = {
    1024, 1122, 1219, 1314, 1407, 1495, 1580, 1658, 1731, 1797, 1855,
    1906, 1948, 1981, 2005, 2019, 2024, 2019, 2005, 1981, 1948, 1906,
    1855, 1797, 1731, 1658, 1580, 1495, 1407, 1314, 1219, 1122, 1024,
    926, 829, 734, 641, 553, 468, 390, 317, 251, 193, 142,
    100, 67, 43, 29, 24, 29, 43, 67, 100, 142, 193,
    251, 317, 390, 468, 553, 641, 734, 829, 926};


Note Song_ch1[] = {{D5, 109}, {REST, 5}, {C5, 109}, {REST, 4}, {A4, 109}, {REST, 5}, {D5, 109}, {REST, 118}, {C5, 109}, {REST, 5}, {A4, 109}, {REST, 118}, {D5, 109}, {REST, 5}, {C5, 109}, {REST, 4}, {A4, 109}, {REST, 5}, {D5, 109}, {REST, 118}, {C5, 109}, {REST, 5}, {A4, 109}, {REST, 118}, {D5, 109}, {REST, 5}, {C5, 109}, {REST, 4}, {D5, 109}, {REST, 5}, {A5, 109}, {REST, 118}, {F5, 109}, {REST, 5}, {D5, 223}, {REST, 4}, {C5, 223}, {REST, 686}, {E4, 223}, {GS4, 223}, {REST, 5}, {F4, 109}, {A4, 109}, {REST, 118}, {A4, 223}, {C5, 223}, {REST, 4}, {A4, 109}, {D5, 109}, {REST, 118}, {C5, 109}, {F5, 109}, {REST, 119}, {C5, 450}, {F5, 450}, {REST, 4}, {E5, 109}, {A5, 109}, {REST, 119}, {FS5, 223}, {AS5, 223}, {REST, 4}, {D5, 109}, {A5, 109}, {REST, 118}, {C5, 109}, {FS5, 109}, {REST, 118}, {A4, 109}, {D5, 109}, {REST, 119}, {FS4, 606}, {C5, 606}, {REST, 75}, {GS5, 52}, {REST, 5}, {E5, 52}, {A5, 52}, {REST, 119}, {D5, 223}, {A5, 223}, {REST, 4}, {D5, 109}, {G5, 109}, {REST, 118}, {D5, 223}, {A5, 223}, {REST, 5}, {D5, 109}, {G5, 109}, {REST, 118}, {D5, 606}, {A5, 606}, {REST, 76}, {D5, 109}, {G5, 109}, {REST, 118}, {A5, 223}, {C6, 223}, {REST, 4}, {E5, 109}, {A5, 109}, {REST, 118}, {D5, 109}, {G5, 109}, {REST, 119}, {C5, 109}, {E5, 109}, {REST, 118}, {AS4, 606}, {D5, 606}, {REST, 76}, {A4, 109}, {C5, 109}, {REST, 118}, {E4, 223}, {GS4, 223}, {REST, 4}, {F4, 109}, {A4, 109}, {REST, 119}, {A4, 223}, {C5, 223}, {REST, 4}, {A4, 109}, {D5, 109}, {REST, 118}, {C5, 109}, {F5, 109}, {REST, 118}, {C5, 450}, {F5, 450}, {REST, 5}, {E5, 109}, {A5, 109}, {REST, 118}, {FS5, 223}, {AS5, 223}, {REST, 5}, {D5, 109}, {A5, 109}, {REST, 118}, {C5, 109}, {FS5, 109}, {REST, 118}, {A5, 109}, {D6, 109}, {REST, 118}, {D5, 606}, {A5, 606}, {REST, 76}, {E5, 109}, {A5, 109}, {REST, 118}, {D5, 223}, {A5, 223}, {REST, 5}, {AS4, 109}, {D5, 109}, {REST, 118}, {D5, 109}, {A5, 109}, {REST, 118}, {E5, 109}, {C6, 109}, {REST, 119}, {C5, 379}, {A5, 379}, {REST, 75}, {G4, 109}, {C5, 109}, {REST, 118}, {A4, 109}, {A5, 109}, {REST, 119}, {A4, 1586}, {F5, 1586}, {REST, 232}, {E4, 223}, {GS4, 223}, {REST, 4}, {F4, 109}, {A4, 109}, {REST, 118}, {A4, 223}, {C5, 223}, {REST, 5}, {A4, 109}, {D5, 109}, {REST, 118}, {C5, 109}, {F5, 109}, {REST, 118}, {C5, 450}, {F5, 450}, {REST, 5}, {E5, 109}, {A5, 109}, {REST, 118}, {FS5, 223}, {AS5, 223}, {REST, 4}, {D5, 109}, {A5, 109}, {REST, 119}, {C5, 109}, {FS5, 109}, {REST, 118}, {A4, 109}, {D5, 109}, {REST, 118}, {FS4, 606}, {C5, 606}, {REST, 76}, {GS5, 52}, {REST, 5}, {E5, 52}, {A5, 52}, {REST, 118}, {D5, 223}, {A5, 223}, {REST, 4}, {D5, 109}, {G5, 109}, {REST, 119}, {D5, 223}, {A5, 223}, {REST, 4}, {D5, 109}, {G5, 109}, {REST, 118}, {D5, 606}, {A5, 606}, {REST, 76}, {D5, 109}, {G5, 109}, {REST, 118}, {A5, 223}, {C6, 223}, {REST, 5}, {E5, 109}, {A5, 109}, {REST, 118}, {D5, 109}, {G5, 109}, {REST, 118}, {C5, 109}, {E5, 109}, {REST, 118}, {AS4, 606}, {D5, 606}, {REST, 76}, {A4, 109}, {C5, 109}, {REST, 119}, {E4, 223}, {GS4, 223}, {REST, 4}, {F4, 109}, {A4, 109}, {REST, 118}, {A4, 223}, {C5, 223}, {REST, 4}, {A4, 109}, {D5, 109}, {REST, 119}, {C5, 109}, {F5, 109}, {REST, 118}, {C5, 450}, {F5, 450}, {REST, 4}, {E5, 109}, {A5, 109}, {REST, 119}, {FS5, 223}, {AS5, 223}, {REST, 4}, {D5, 109}, {A5, 109}, {REST, 118}, {C5, 109}, {FS5, 109}, {REST, 119}, {A5, 109}, {D6, 109}, {REST, 118}, {D5, 606}, {A5, 606}, {REST, 76}, {E5, 109}, {A5, 109}, {REST, 118}, {D5, 223}, {A5, 223}, {REST, 4}, {AS4, 109}, {D5, 109}, {REST, 118}, {D5, 109}, {A5, 109}, {REST, 119}, {E5, 109}, {C6, 109}, {REST, 118}, {C5, 379}, {A5, 379}, {REST, 76}, {G4, 109}, {C5, 109}, {REST, 118}, {A4, 109}, {A5, 109}, {REST, 118}, {A4, 1586}, {F5, 1586}, {REST, 232}, {D5, 109}, {REST, 5}, {C5, 109}, {REST, 118}, {D5, 109}, {REST, 5}, {C5, 109}, {REST, 118}, {D5, 109}, {REST, 5}, {C5, 109}, {REST, 118}, {D5, 109}, {REST, 5}, {C5, 109}, {REST, 118}, {D5, 223}, {REST, 4}, {A4, 223}, {REST, 4}, {C5, 109}, {REST, 5}, {AS4, 109}, {REST, 118}, {C5, 109}, {REST, 5}, {AS4, 109}, {REST, 118}, {C5, 109}, {REST, 5}, {AS4, 336}, {REST, 687}, {C5, 109}, {REST, 4}, {AS4, 109}, {REST, 119}, {C5, 109}, {REST, 4}, {AS4, 109}, {REST, 118}, {C5, 109}, {REST, 5}, {AS4, 109}, {REST, 118}, {C5, 109}, {REST, 5}, {AS4, 109}, {REST, 118}, {C5, 223}, {REST, 5}, {AS4, 223}, {REST, 4}, {AS4, 109}, {REST, 4}, {A4, 109}, {REST, 119}, {AS4, 109}, {REST, 4}, {A4, 109}, {REST, 119}, {AS4, 109}, {REST, 4}, {A4, 336}, {REST, 687}, {D5, 109}, {REST, 5}, {C5, 109}, {REST, 118}, {D5, 109}, {REST, 5}, {C5, 109}, {REST, 118}, {D5, 109}, {REST, 4}, {C5, 109}, {REST, 119}, {D5, 109}, {REST, 4}, {C5, 109}, {REST, 119}, {D5, 223}, {REST, 4}, {A4, 223}, {REST, 4}, {C5, 109}, {REST, 5}, {AS4, 109}, {REST, 118}, {C5, 109}, {REST, 5}, {AS4, 109}, {REST, 118}, {C5, 109}, {REST, 5}, {AS4, 336}, {REST, 686}, {E4, 109}, {G4, 109}, {C5, 109}, {REST, 119}, {E4, 336}, {G4, 336}, {C5, 336}, {REST, 5}, {E4, 336}, {G4, 336}, {C5, 336}, {REST, 4}, {D4, 109}, {F4, 109}, {AS4, 109}, {REST, 119}, {D4, 336}, {F4, 336}, {AS4, 336}, {REST, 5}, {D4, 336}, {F4, 336}, {AS4, 336}, {REST, 4}, {C4, 109}, {E4, 109}, {A4, 109}, {REST, 119}, {C4, 336}, {E4, 336}, {A4, 336}, {REST, 5}, {C4, 336}, {E4, 336}, {A4, 336}, {REST, 5}, {AS3, 109}, {D4, 109}, {G4, 109}, {REST, 118}, {E3, 606}, {AS3, 606}, {C4, 606}, {REST, 76}, {E4, 223}, {GS4, 223}, {REST, 4}, {F4, 109}, {A4, 109}, {REST, 118}, {A4, 223}, {C5, 223}, {REST, 4}, {A4, 109}, {D5, 109}, {REST, 119}, {C5, 109}, {F5, 109}, {REST, 118}, {C5, 450}, {F5, 450}, {REST, 5}, {E5, 109}, {A5, 109}, {REST, 118}, {FS5, 223}, {AS5, 223}, {REST, 4}, {D5, 109}, {A5, 109}, {REST, 118}, {C5, 109}, {FS5, 109}, {REST, 119}, {A5, 109}, {D6, 109}, {REST, 118}, {D5, 606}, {A5, 606}, {REST, 76}, {E5, 109}, {A5, 109}, {REST, 118}, {D5, 223}, {A5, 223}, {REST, 4}, {AS4, 109}, {D5, 109}, {REST, 119}, {D5, 109}, {A5, 109}, {REST, 118}, {E5, 109}, {C6, 109}, {REST, 118}, {C5, 379}, {A5, 379}, {REST, 76}, {G4, 109}, {C5, 109}, {REST, 118}, {A4, 109}, {A5, 109}, {REST, 118}, {A4, 1586}, {F5, 1586}, {REST, 232}, {F4, 76}, {A4, 76}, {C5, 76}, {REST, 152}, {F4, 76}, {A4, 76}, {C5, 76}, {REST, 151}, {F4, 76}, {A4, 76}, {C5, 76}, {REST, 151}, {F4, 76}, {A4, 76}, {C5, 76}, {REST, 151}, {F4, 76}, {A4, 76}, {C5, 76}, {REST, 152}, {F4, 76}, {A4, 76}, {C5, 76}, {REST, 37}, {F4, 76}, {A4, 76}, {C5, 76}, {REST, 152}, {F4, 76}, {A4, 76}, {C5, 76}, {REST, 37}, {F4, 76}, {A4, 76}, {C5, 76}, {REST, 152}, {FS4, 76}, {A4, 76}, {D5, 76}, {REST, 151}, {FS4, 76}, {A4, 76}, {D5, 76}, {REST, 151}, {FS4, 76}, {A4, 76}, {D5, 76}, {REST, 151}, {FS4, 76}, {A4, 76}, {D5, 76}, {REST, 152}, {FS4, 76}, {A4, 76}, {D5, 76}, {REST, 151}, {FS4, 76}, {A4, 76}, {D5, 76}, {REST, 38}, {FS4, 76}, {A4, 76}, {D5, 76}, {REST, 151}, {FS4, 76}, {A4, 76}, {D5, 76}, {REST, 37}, {FS4, 76}, {A4, 76}, {D5, 76}, {REST, 152}, {F4, 76}, {AS4, 76}, {D5, 76}, {REST, 151}, {F4, 76}, {AS4, 76}, {D5, 76}, {REST, 151}, {F4, 76}, {AS4, 76}, {D5, 76}, {REST, 152}, {F4, 76}, {AS4, 76}, {D5, 76}, {REST, 151}, {F4, 76}, {AS4, 76}, {D5, 76}, {REST, 151}, {F4, 76}, {AS4, 76}, {D5, 76}, {REST, 38}, {F4, 76}, {AS4, 76}, {D5, 76}, {REST, 151}, {F4, 76}, {AS4, 76}, {D5, 76}, {REST, 38}, {F4, 76}, {AS4, 76}, {D5, 76}, {REST, 151}, {E4, 76}, {AS4, 76}, {C5, 76}, {REST, 151}, {E4, 76}, {AS4, 76}, {C5, 76}, {REST, 151}, {E4, 76}, {AS4, 76}, {C5, 76}, {REST, 152}, {E4, 76}, {AS4, 76}, {C5, 76}, {REST, 151}, {E4, 76}, {AS4, 76}, {C5, 76}, {REST, 151}, {E4, 76}, {AS4, 76}, {C5, 76}, {REST, 38}, {E4, 76}, {AS4, 76}, {C5, 76}, {REST, 151}, {E4, 76}, {AS4, 76}, {C5, 76}, {REST, 38}, {E4, 76}, {AS4, 76}, {C5, 76}, {REST, 151}, {F4, 109}, {REST, 5}, {C4, 109}, {REST, 4}, {F4, 109}, {REST, 5}, {G4, 109}, {REST, 5}, {GS4, 109}, {REST, 4}, {A4, 109}, {REST, 5}, {F4, 109}, {REST, 5}, {C4, 109}, {REST, 4}, {F4, 109}, {REST, 5}, {C4, 109}, {REST, 4}, {F4, 109}, {REST, 5}, {A4, 336}, {REST, 5}, {F4, 109}, {REST, 5}, {C4, 109}, {REST, 4}, {FS4, 109}, {REST, 5}, {D4, 109}, {REST, 5}, {FS4, 109}, {REST, 4}, {A4, 109}, {REST, 5}, {C5, 223}, {REST, 4}, {A4, 109}, {REST, 5}, {FS4, 109}, {REST, 4}, {D5, 109}, {REST, 5}, {A4, 109}, {REST, 5}, {FS4, 109}, {REST, 4}, {D4, 109}, {REST, 5}, {FS4, 109}, {REST, 5}, {A4, 109}, {REST, 4}, {FS4, 109}, {REST, 5}, {D4, 109}, {REST, 4}, {G4, 223}, {REST, 5}, {D4, 223}, {REST, 4}, {G4, 109}, {REST, 5}, {D4, 109}, {REST, 4}, {G4, 109}, {REST, 5}, {AS4, 109}, {REST, 5}, {AS4, 109}, {REST, 4}, {C5, 109}, {REST, 5}, {AS4, 109}, {REST, 4}, {A4, 109}, {REST, 5}, {G4, 109}, {REST, 5}, {F4, 109}, {REST, 4}, {E4, 109}, {REST, 5}, {D4, 109}, {REST, 5}, {E4, 109}, {REST, 4}, {D4, 109}, {REST, 5}, {E4, 109}, {REST, 5}, {C4, 109}, {REST, 4}, {E4, 109}, {REST, 5}, {C4, 109}, {REST, 4}, {E4, 109}, {REST, 5}, {G4, 109}, {REST, 5}, {AS4, 109}, {REST, 4}, {C5, 109}, {REST, 5}, {AS4, 109}, {REST, 5}, {A4, 109}, {REST, 4}, {G4, 109}, {REST, 5}, {F4, 109}, {REST, 5}, {E4, 109}, {REST, 4}, {D4, 109}, {REST, 5}, {F4, 109}, {REST, 4}, {C4, 109}, {REST, 5}, {F4, 109}, {REST, 5}, {G4, 109}, {REST, 4}, {GS4, 109}, {REST, 5}, {A4, 109}, {REST, 5}, {F4, 109}, {REST, 4}, {C4, 109}, {REST, 5}, {F4, 109}, {REST, 5}, {C4, 109}, {REST, 4}, {F4, 109}, {REST, 5}, {A4, 336}, {REST, 5}, {F4, 109}, {REST, 4}, {C4, 109}, {REST, 5}, {FS4, 109}, {REST, 5}, {D4, 109}, {REST, 4}, {FS4, 109}, {REST, 5}, {A4, 109}, {REST, 5}, {C5, 223}, {REST, 4}, {A4, 109}, {REST, 4}, {FS4, 109}, {REST, 5}, {D5, 109}, {REST, 5}, {A4, 109}, {REST, 4}, {FS4, 109}, {REST, 5}, {D4, 109}, {REST, 5}, {FS4, 109}, {REST, 4}, {A4, 109}, {REST, 5}, {FS4, 109}, {REST, 5}, {D4, 109}, {REST, 4}, {G4, 109}, {REST, 5}, {FS4, 109}, {REST, 4}, {G4, 109}, {REST, 5}, {A4, 109}, {REST, 5}, {G4, 109}, {REST, 4}, {D4, 109}, {REST, 5}, {G4, 109}, {REST, 5}, {AS4, 109}, {REST, 4}, {E4, 109}, {REST, 5}, {C4, 109}, {REST, 5}, {E4, 109}, {REST, 4}, {C4, 109}, {REST, 5}, {E4, 109}, {REST, 4}, {C4, 109}, {REST, 5}, {E4, 109}, {REST, 5}, {G4, 109}, {REST, 4}, {F4, 109}, {REST, 5}, {D4, 109}, {REST, 5}, {F4, 109}, {REST, 4}, {G4, 109}, {REST, 5}, {GS4, 109}, {REST, 5}, {A4, 109}, {REST, 4}, {F4, 109}, {REST, 5}, {C4, 109}, {REST, 4}, {F4, 109}, {REST, 5}, {C4, 109}, {REST, 5}, {D4, 109}, {REST, 4}, {F4, 223}, {REST, 5}, {C4, 109}, {REST, 4}, {D4, 109}, {REST, 5}, {F4, 109}, {REST, 5}, {E4, 223}, {GS4, 223}, {REST, 4}, {F4, 109}, {A4, 109}, {REST, 118}, {A4, 223}, {C5, 223}, {REST, 4}, {A4, 109}, {D5, 109}, {REST, 119}, {C5, 109}, {F5, 109}, {REST, 118}, {C5, 450}, {F5, 450}, {REST, 4}, {E5, 109}, {A5, 109}, {REST, 119}, {FS5, 223}, {AS5, 223}, {REST, 4}, {D5, 109}, {A5, 109}, {REST, 118}, {C5, 109}, {FS5, 109}, {REST, 119}, {A4, 109}, {D5, 109}, {REST, 118}, {FS4, 606}, {C5, 606}, {REST, 76}, {GS5, 52}, {REST, 4}, {E5, 52}, {A5, 52}, {REST, 119}, {D5, 223}, {A5, 223}, {REST, 4}, {D5, 109}, {G5, 109}, {REST, 118}, {D5, 223}, {A5, 223}, {REST, 5}, {D5, 109}, {G5, 109}, {REST, 118}, {D5, 606}, {A5, 606}, {REST, 76}, {D5, 109}, {G5, 109}, {REST, 118}, {A5, 223}, {C6, 223}, {REST, 4}, {E5, 109}, {A5, 109}, {REST, 119}, {D5, 109}, {G5, 109}, {REST, 118}, {C5, 109}, {E5, 109}, {REST, 118}, {AS4, 606}, {D5, 606}, {REST, 76}, {A4, 109}, {C5, 109}, {REST, 118}, {E4, 223}, {GS4, 223}, {REST, 5}, {F4, 109}, {A4, 109}, {REST, 118}, {A4, 223}, {C5, 223}, {REST, 4}, {A4, 109}, {D5, 109}, {REST, 118}, {C5, 109}, {F5, 109}, {REST, 119}, {C5, 450}, {F5, 450}, {REST, 4}, {E5, 109}, {A5, 109}, {REST, 118}, {FS5, 223}, {AS5, 223}, {REST, 5}, {D5, 109}, {A5, 109}, {REST, 118}, {C5, 109}, {FS5, 109}, {REST, 118}, {A5, 109}, {D6, 109}, {REST, 119}, {D5, 606}, {A5, 606}, {REST, 75}, {E5, 109}, {A5, 109}, {REST, 119}, {D5, 223}, {A5, 223}, {REST, 4}, {AS4, 109}, {D5, 109}, {REST, 118}, {D5, 109}, {A5, 109}, {REST, 118}, {E5, 109}, {C6, 109}, {REST, 119}, {C5, 379}, {A5, 379}, {REST, 75}, {G4, 109}, {C5, 109}, {REST, 119}, {A4, 109}, {A5, 109}, {REST, 118}, {A4, 1586}, {F5, 1586}, {REST, 232}, {D5, 109}, {REST, 5}, {C5, 109}, {REST, 118}, {D5, 109}, {REST, 5}, {C5, 109}, {REST, 118}, {D5, 109}, {REST, 4}, {C5, 109}, {REST, 119}, {D5, 109}, {REST, 4}, {C5, 109}, {REST, 119}, {D5, 223}, {REST, 4}, {A4, 223}, {REST, 4}, {C5, 109}, {REST, 5}, {AS4, 109}, {REST, 118}, {C5, 109}, {REST, 5}, {AS4, 109}, {REST, 118}, {C5, 109}, {REST, 5}, {AS4, 336}, {REST, 686}, {C5, 109}, {REST, 5}, {AS4, 109}, {REST, 118}, {C5, 109}, {REST, 5}, {AS4, 109}, {REST, 118}, {C5, 109}, {REST, 5}, {AS4, 109}, {REST, 118}, {C5, 109}, {REST, 5}, {AS4, 109}, {REST, 118}, {C5, 223}, {REST, 4}, {AS4, 223}, {REST, 5}, {AS4, 109}, {REST, 4}, {A4, 109}, {REST, 118}, {AS4, 109}, {REST, 5}, {A4, 109}, {REST, 118}, {AS4, 109}, {REST, 5}, {A4, 336}, {REST, 687}, {D5, 109}, {REST, 4}, {C5, 109}, {REST, 119}, {D5, 109}, {REST, 4}, {C5, 109}, {REST, 119}, {D5, 109}, {REST, 4}, {C5, 109}, {REST, 118}, {D5, 109}, {REST, 5}, {C5, 109}, {REST, 118}, {D5, 223}, {REST, 5}, {A4, 223}, {REST, 4}, {C5, 109}, {REST, 5}, {AS4, 109}, {REST, 118}, {C5, 109}, {REST, 4}, {AS4, 109}, {REST, 119}, {C5, 109}, {REST, 4}, {AS4, 336}, {REST, 687}, {E4, 109}, {G4, 109}, {C5, 109}, {REST, 118}, {E4, 336}, {G4, 336}, {C5, 336}, {REST, 5}, {E4, 336}, {G4, 336}, {C5, 336}, {REST, 5}, {D4, 109}, {F4, 109}, {AS4, 109}, {REST, 118}, {D4, 336}, {F4, 336}, {AS4, 336}, {REST, 5}, {D4, 336}, {F4, 336}, {AS4, 336}, {REST, 5}, {C4, 109}, {E4, 109}, {A4, 109}, {REST, 119}, {C4, 336}, {E4, 336}, {A4, 336}, {REST, 4}, {C4, 336}, {E4, 336}, {A4, 336}, {REST, 5}, {AS3, 109}, {D4, 109}, {G4, 109}, {REST, 119}, {E3, 606}, {AS3, 606}, {C4, 606}, {REST, 75}, {E4, 223}, {GS4, 223}, {REST, 5}, {F4, 109}, {A4, 109}, {REST, 118}, {A4, 223}, {C5, 223}, {REST, 4}, {A4, 109}, {D5, 109}, {REST, 119}, {C5, 109}, {F5, 109}, {REST, 118}, {C5, 450}, {F5, 450}, {REST, 4}, {E5, 109}, {A5, 109}, {REST, 119}, {FS5, 223}, {AS5, 223}, {REST, 4}, {D5, 109}, {A5, 109}, {REST, 118}, {C5, 109}, {FS5, 109}, {REST, 118}, {A5, 109}, {D6, 109}, {REST, 119}, {D5, 606}, {A5, 606}, {REST, 76}, {E5, 109}, {A5, 109}, {REST, 118}, {D5, 223}, {A5, 223}, {REST, 4}, {AS4, 109}, {D5, 109}, {REST, 118}, {D5, 109}, {A5, 109}, {REST, 119}, {E5, 109}, {C6, 109}, {REST, 118}, {C5, 379}, {A5, 379}, {REST, 75}, {G4, 109}, {C5, 109}, {REST, 119}, {A4, 109}, {A5, 109}, {REST, 118}, {A4, 1586}, {F5, 1586}, {REST, 232}, {F4, 76}, {A4, 76}, {C5, 76}, {REST, 151}, {F4, 76}, {A4, 76}, {C5, 76}, {REST, 152}, {F4, 76}, {A4, 76}, {C5, 76}, {REST, 151}, {F4, 76}, {A4, 76}, {C5, 76}, {REST, 151}, {F4, 76}, {A4, 76}, {C5, 76}, {REST, 152}, {F4, 76}, {A4, 76}, {C5, 76}, {REST, 37}, {F4, 76}, {A4, 76}, {C5, 76}, {REST, 151}, {F4, 76}, {A4, 76}, {C5, 76}, {REST, 38}, {F4, 76}, {A4, 76}, {C5, 76}, {REST, 151}, {FS4, 76}, {A4, 76}, {D5, 76}, {REST, 152}, {FS4, 76}, {A4, 76}, {D5, 76}, {REST, 151}, {FS4, 76}, {A4, 76}, {D5, 76}, {REST, 151}, {FS4, 76}, {A4, 76}, {D5, 76}, {REST, 151}, {FS4, 76}, {A4, 76}, {D5, 76}, {REST, 152}, {FS4, 76}, {A4, 76}, {D5, 76}, {REST, 37}, {FS4, 76}, {A4, 76}, {D5, 76}, {REST, 152}, {FS4, 76}, {A4, 76}, {D5, 76}, {REST, 37}, {FS4, 76}, {A4, 76}, {D5, 76}, {REST, 152}, {F4, 76}, {AS4, 76}, {D5, 76}, {REST, 151}, {F4, 76}, {AS4, 76}, {D5, 76}, {REST, 151}, {F4, 76}, {AS4, 76}, {D5, 76}, {REST, 151}, {F4, 76}, {AS4, 76}, {D5, 76}, {REST, 152}, {F4, 76}, {AS4, 76}, {D5, 76}, {REST, 151}, {F4, 76}, {AS4, 76}, {D5, 76}, {REST, 38}, {F4, 76}, {AS4, 76}, {D5, 76}, {REST, 151}, {F4, 76}, {AS4, 76}, {D5, 76}, {REST, 37}, {F4, 76}, {AS4, 76}, {D5, 76}, {REST, 152}, {E4, 76}, {AS4, 76}, {C5, 76}, {REST, 151}, {E4, 76}, {AS4, 76}, {C5, 76}, {REST, 151}, {E4, 76}, {AS4, 76}, {C5, 76}, {REST, 152}, {E4, 76}, {AS4, 76}, {C5, 76}, {REST, 151}, {E4, 76}, {AS4, 76}, {C5, 76}, {REST, 151}, {E4, 76}, {AS4, 76}, {C5, 76}, {REST, 38}, {E4, 76}, {AS4, 76}, {C5, 76}, {REST, 151}, {E4, 76}, {AS4, 76}, {C5, 76}, {REST, 38}, {E4, 76}, {AS4, 76}, {C5, 76}, {REST, 151}, {F4, 109}, {REST, 5}, {C4, 109}, {REST, 4}, {F4, 109}, {REST, 5}, {G4, 109}, {REST, 4}, {GS4, 109}, {REST, 5}, {A4, 109}, {REST, 5}, {F4, 109}, {REST, 4}, {C4, 109}, {REST, 5}, {F4, 109}, {REST, 5}, {C4, 109}, {REST, 4}, {F4, 109}, {REST, 5}, {A4, 336}, {REST, 5}, {F4, 109}, {REST, 4}, {C4, 109}, {REST, 5}, {FS4, 109}, {REST, 5}, {D4, 109}, {REST, 4}, {FS4, 109}, {REST, 5}, {A4, 109}, {REST, 5}, {C5, 223}, {REST, 4}, {A4, 109}, {REST, 5}, {FS4, 109}, {REST, 4}, {D5, 109}, {REST, 5}, {A4, 109}, {REST, 4}, {FS4, 109}, {REST, 5}, {D4, 109}, {REST, 5}, {FS4, 109}, {REST, 4}, {A4, 109}, {REST, 5}, {FS4, 109}, {REST, 5}, {D4, 109}, {REST, 4}, {G4, 223}, {REST, 5}, {D4, 223}, {REST, 4}, {G4, 109}, {REST, 4}, {D4, 109}, {REST, 5}, {G4, 109}, {REST, 5}, {AS4, 109}, {REST, 4}, {AS4, 109}, {REST, 5}, {C5, 109}, {REST, 5}, {AS4, 109}, {REST, 4}, {A4, 109}, {REST, 5}, {G4, 109}, {REST, 5}, {F4, 109}, {REST, 4}, {E4, 109}, {REST, 5}, {D4, 109}, {REST, 4}, {E4, 109}, {REST, 5}, {D4, 109}, {REST, 5}, {E4, 109}, {REST, 4}, {C4, 109}, {REST, 5}, {E4, 109}, {REST, 5}, {C4, 109}, {REST, 4}, {E4, 109}, {REST, 5}, {G4, 109}, {REST, 5}, {AS4, 109}, {REST, 4}, {C5, 109}, {REST, 5}, {AS4, 109}, {REST, 4}, {A4, 109}, {REST, 5}, {G4, 109}, {REST, 5}, {F4, 109}, {REST, 4}, {E4, 109}, {REST, 5}, {D4, 109}, {REST, 5}, {F4, 109}, {REST, 4}, {C4, 109}, {REST, 5}, {F4, 109}, {REST, 5}, {G4, 109}, {REST, 4}, {GS4, 109}, {REST, 5}, {A4, 109}, {REST, 4}, {F4, 109}, {REST, 5}, {C4, 109}, {REST, 5}, {F4, 109}, {REST, 4}, {C4, 109}, {REST, 5}, {F4, 109}, {REST, 5}, {A4, 336}, {REST, 5}, {F4, 109}, {REST, 4}, {C4, 109}, {REST, 5}, {FS4, 109}, {REST, 4}, {D4, 109}, {REST, 5}, {FS4, 109}, {REST, 5}, {A4, 109}, {REST, 4}, {C5, 223}, {REST, 5}, {A4, 109}, {REST, 4}, {FS4, 109}, {REST, 5}, {D5, 109}, {REST, 5}, {A4, 109}, {REST, 4}, {FS4, 109}, {REST, 5}, {D4, 109}, {REST, 4}, {FS4, 109}, {REST, 5}, {A4, 109}, {REST, 5}, {FS4, 109}, {REST, 4}, {D4, 109}, {REST, 5}, {G4, 109}, {REST, 5}, {FS4, 109}, {REST, 4}, {G4, 109}, {REST, 5}, {A4, 109}, {REST, 5}, {G4, 109}, {REST, 4}, {D4, 109}, {REST, 5}, {G4, 109}, {REST, 4}, {AS4, 109}, {REST, 5}, {E4, 109}, {REST, 5}, {C4, 109}, {REST, 4}, {E4, 109}, {REST, 5}, {C4, 109}, {REST, 5}, {E4, 109}, {REST, 4}, {C4, 109}, {REST, 5}, {E4, 109}, {REST, 5}, {G4, 109}, {REST, 4}, {F4, 109}, {REST, 5}, {D4, 109}, {REST, 4}, {F4, 109}, {REST, 5}, {G4, 109}, {REST, 5}, {GS4, 109}, {REST, 4}, {A4, 109}, {REST, 5}, {F4, 109}, {REST, 5}, {C4, 109}, {REST, 4}, {F4, 109}, {REST, 5}, {C4, 109}, {REST, 5}, {D4, 109}, {REST, 4}, {F4, 223}, {REST, 4}, {C4, 109}, {REST, 5}, {D4, 109}, {REST, 5}, {F4, 109}};
#define SONG_CH1_LENGTH (sizeof(Song_ch1) / sizeof(Song_ch1[0]))

Note Song_ch2[] = {{F2, 223}, {A2, 223}, {REST, 459}, {F2, 223}, {A2, 223}, {REST, 4}, {FS2, 223}, {A2, 223}, {REST, 459}, {FS2, 223}, {A2, 223}, {REST, 4}, {G2, 223}, {AS2, 223}, {REST, 459}, {G2, 223}, {AS2, 223}, {REST, 4}, {C2, 223}, {C3, 223}, {REST, 5}, {C2, 223}, {REST, 4}, {D2, 223}, {REST, 4}, {E2, 223}, {REST, 4}, {F2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {D2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {FS2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {D2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {FS2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {G2, 223}, {REST, 4}, {AS2, 109}, {REST, 118}, {D2, 223}, {REST, 5}, {AS2, 109}, {REST, 118}, {G2, 223}, {REST, 4}, {AS2, 109}, {REST, 118}, {D2, 223}, {REST, 5}, {AS2, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {G2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {D2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {FS2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {D2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {FS2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 5}, {AS2, 109}, {REST, 118}, {D2, 223}, {REST, 4}, {AS2, 109}, {REST, 119}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {D2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {FS2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {D2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {FS2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 4}, {AS2, 109}, {REST, 119}, {D2, 223}, {REST, 4}, {AS2, 109}, {REST, 118}, {G2, 223}, {REST, 5}, {AS2, 109}, {REST, 118}, {D2, 223}, {REST, 4}, {AS2, 109}, {REST, 118}, {C2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {D2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {FS2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {D2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {FS2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 4}, {AS2, 109}, {REST, 118}, {D2, 223}, {REST, 5}, {AS2, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {G2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {D2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {FS2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {D2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {FS2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 5}, {AS2, 109}, {REST, 118}, {D2, 223}, {REST, 4}, {AS2, 109}, {REST, 119}, {G2, 223}, {REST, 4}, {F2, 223}, {REST, 4}, {E2, 223}, {REST, 4}, {D2, 223}, {REST, 5}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 4}, {F2, 223}, {REST, 4}, {E2, 223}, {REST, 5}, {DS2, 223}, {REST, 4}, {D2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {FS2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {D2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {FS2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 4}, {AS2, 109}, {REST, 119}, {D2, 223}, {REST, 4}, {AS2, 109}, {REST, 118}, {G2, 223}, {REST, 5}, {F2, 223}, {REST, 4}, {E2, 223}, {REST, 4}, {D2, 223}, {REST, 4}, {C3, 223}, {REST, 5}, {C3, 336}, {REST, 5}, {C3, 336}, {REST, 4}, {AS2, 223}, {REST, 5}, {AS2, 336}, {REST, 5}, {AS2, 336}, {REST, 4}, {A2, 223}, {REST, 5}, {A2, 336}, {REST, 5}, {A2, 336}, {REST, 5}, {G2, 223}, {REST, 4}, {C2, 223}, {REST, 4}, {D2, 223}, {REST, 4}, {E2, 223}, {REST, 5}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {D2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {FS2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {D2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {FS2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 4}, {AS2, 109}, {REST, 119}, {D2, 223}, {REST, 4}, {AS2, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {G2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {D2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {FS2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {D2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {FS2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {G2, 223}, {REST, 4}, {AS2, 109}, {REST, 118}, {D2, 223}, {REST, 5}, {AS2, 109}, {REST, 118}, {G2, 223}, {REST, 4}, {AS2, 109}, {REST, 118}, {D2, 223}, {REST, 5}, {AS2, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {G2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {D2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {FS2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {D2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {FS2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 5}, {AS2, 109}, {REST, 118}, {D2, 223}, {REST, 4}, {AS2, 109}, {REST, 119}, {G2, 223}, {REST, 4}, {AS2, 109}, {REST, 118}, {D2, 223}, {REST, 4}, {AS2, 109}, {REST, 119}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {D2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {FS2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {D2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {FS2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 4}, {AS2, 109}, {REST, 119}, {D2, 223}, {REST, 4}, {AS2, 109}, {REST, 118}, {C2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {D2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {FS2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {D2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {FS2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 4}, {AS2, 109}, {REST, 118}, {D2, 223}, {REST, 5}, {AS2, 109}, {REST, 118}, {G2, 223}, {REST, 4}, {AS2, 109}, {REST, 119}, {D2, 223}, {REST, 4}, {AS2, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {G2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {G2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {D2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {FS2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {D2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {FS2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {G2, 223}, {REST, 4}, {AS2, 109}, {REST, 118}, {D2, 223}, {REST, 4}, {AS2, 109}, {REST, 119}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {D2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {FS2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {D2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {FS2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 4}, {AS2, 109}, {REST, 119}, {D2, 223}, {REST, 4}, {AS2, 109}, {REST, 118}, {G2, 223}, {REST, 5}, {F2, 223}, {REST, 4}, {E2, 223}, {REST, 4}, {D2, 223}, {REST, 4}, {C2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {F2, 223}, {REST, 4}, {F2, 223}, {REST, 4}, {E2, 223}, {REST, 4}, {DS2, 223}, {REST, 5}, {D2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {FS2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {D2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {FS2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 4}, {AS2, 109}, {REST, 118}, {D2, 223}, {REST, 5}, {AS2, 109}, {REST, 118}, {G2, 223}, {REST, 4}, {F2, 223}, {REST, 5}, {E2, 223}, {REST, 4}, {D2, 223}, {REST, 4}, {C3, 223}, {REST, 4}, {C3, 336}, {REST, 5}, {C3, 336}, {REST, 5}, {AS2, 223}, {REST, 4}, {AS2, 336}, {REST, 5}, {AS2, 336}, {REST, 5}, {A2, 223}, {REST, 5}, {A2, 336}, {REST, 4}, {A2, 336}, {REST, 5}, {G2, 223}, {REST, 5}, {C2, 223}, {REST, 4}, {D2, 223}, {REST, 4}, {E2, 223}, {REST, 4}, {F2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {D2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {FS2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {D2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {FS2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 4}, {AS2, 109}, {REST, 118}, {D2, 223}, {REST, 5}, {AS2, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {D2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {FS2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {D2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {FS2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {G2, 223}, {REST, 4}, {AS2, 109}, {REST, 118}, {D2, 223}, {REST, 4}, {AS2, 109}, {REST, 119}, {G2, 223}, {REST, 4}, {AS2, 109}, {REST, 118}, {D2, 223}, {REST, 4}, {AS2, 109}, {REST, 119}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {D2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {FS2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {D2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {FS2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 5}, {AS2, 109}, {REST, 118}, {D2, 223}, {REST, 4}, {AS2, 109}, {REST, 118}, {G2, 223}, {REST, 5}, {AS2, 109}, {REST, 118}, {D2, 223}, {REST, 4}, {AS2, 109}, {REST, 118}, {C2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {D2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {FS2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {D2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {FS2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {G2, 223}, {REST, 4}, {AS2, 109}, {REST, 119}, {D2, 223}, {REST, 4}, {AS2, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {G2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 4}, {C3, 109}, {REST, 119}, {C2, 223}, {REST, 4}, {C3, 109}, {REST, 118}, {F2, 223}, {REST, 5}, {C3, 109}, {REST, 118}, {C2, 223}, {REST, 4}, {C3, 109}};
#define SONG_CH2_LENGTH (sizeof(Song_ch2) / sizeof(Song_ch2[0]))

static uint32_t SongIndex; // Index into the song's notes
static uint32_t SongIndex2;
static uint32_t NoteDuration;
static uint32_t NoteDuration2;  
static uint32_t tempo = 1;
uint32_t tick_count1 = 0;
uint32_t tick_count2 = 0;
uint32_t amp1 = 0;
uint32_t amp2 = 0;

void metronome(void)
{
    tick_count1++;
    tick_count2++;

    if(amp1 < 2048 && tick_count1 < 2048){
        amp1 += 1;
    }
    else if(amp1 > 0 && tick_count1%50 == 0){
        amp1 -= 1;
    }

    if(amp2 < 2048 && tick_count2 < 2048){
        amp2 += 1;
    }
    else if(amp2 > 0 && tick_count2%50 == 0){
        amp2 -= 1;
    }
    
    if(Song_ch1[SongIndex].Pitch != REST && Song_ch2[SongIndex2].Pitch != REST){
        DAC_Out((amp1*Wave[I1] + amp2*Wave[I2])>>11); // output to DAC
    }
    else if(Song_ch2[SongIndex2].Pitch == REST){
        DAC_Out((amp1*(Wave[I1])) >> 10);
    }
    else if(Song_ch1[SongIndex].Pitch == REST){
        DAC_Out((amp2*(Wave[I2])) >> 10);
    }
    else{
        DAC_Out(0);
    }

    NoteDuration--;
    NoteDuration2--;

    if (NoteDuration == 0)
    {
        tick_count1=0;
        SongIndex++;
        if (SongIndex >= SONG_CH1_LENGTH)
        {
            SongIndex = 0; // Loop the song
            I1 = 0;
            amp1=0;
        }
        // Load the next note
        NoteDuration = Song_ch1[SongIndex].Duration*100 / tempo;
        if (Song_ch1[SongIndex].Pitch == REST)
        {
            SysTick_Stop(); // Silence for a rest
        }
        else
        {
            SysTick_Start();
            SysTick_setPeriod(Song_ch1[SongIndex].Pitch);
        }
    }
    if (NoteDuration2 == 0)
    {
        tick_count2=0;
        SongIndex2++;
        if (SongIndex2 >= SONG_CH2_LENGTH)
        {
            SongIndex2 = 0; // Loop the song
            I2 = 0;
            amp2 = 0;
        }
        // Load the next note
        NoteDuration2 = Song_ch2[SongIndex2].Duration*100 / tempo;
        if (Song_ch2[SongIndex2].Pitch == REST)
        {
            NVIC_DIS0_R = 1 << 23;     // 9) disable interrupt 23 in NVIC
            TIMER2_CTL_R = 0x00000000; // 10) disable timer2A
        }
        else
        {
            NVIC_EN0_R = 1 << 23;      // 9) enable IRQ 23 in NVIC
            TIMER2_CTL_R = 0x00000001; // 10) enable timer2A
            TIMER2_TAILR_R = (Song_ch2[SongIndex2].Pitch) - 1;
        }
    }
}

void Pitch2_Handler(void)
{
    I2 = (I2 + 1) & 0x3F;
}

void SysTick_Handler(void)
{
    I1 = (I1 + 1) & 0x3F;
}

//-------------- Music_Init ----------------
// activate periodic interrupts and DAC
// Inputs: none
// Outputs: none
// called once
void Music_Init(void)
{
    DAC_Init(0);
    SongIndex = 0;
    SongIndex2 = 0;
    NoteDuration = Song_ch1[SongIndex].Duration*100 / tempo;
    NoteDuration2 = Song_ch2[SongIndex2].Duration*100 / tempo;
    SysTick_Init(Song_ch1[SongIndex].Pitch);
    Timer2A_Init(Pitch2_Handler, Song_ch2[SongIndex2].Pitch, 2);
    I1 = 0; // instr 1
    I2 = 0; // instr 2
    Timer1A_Init(metronome, 800, 2);
    TIMER1_CTL_R = 0x00000000; // Disable metronome
}

void Play(void)
{
    NoteDuration = Song_ch1[SongIndex].Duration*100 / tempo;
    NoteDuration2 = Song_ch2[SongIndex2].Duration*100 / tempo;
    SysTick_setPeriod(Song_ch1[SongIndex].Pitch);
    SysTick_Start();
    TIMER2_TAILR_R = (Song_ch2[SongIndex2].Pitch) - 1;
    NVIC_EN0_R = 1 << 23;      // 9) enable IRQ 23 in NVIC
    TIMER2_CTL_R = 0x00000001; // 10) enable timer2A
    TIMER1_CTL_R = 0x00000001; // Enable metronome
}

void Stop(void)
{
    SysTick_Stop();
    TIMER1_CTL_R = 0x00000000; // Disable metronome
    NVIC_DIS0_R = 1 << 23;     // 9) disable interrupt 23 in NVIC
    TIMER2_CTL_R = 0x00000000; // 10) disable timer2A
}

void Rewind(void)
{
    amp1=0;
    amp2=0;
    tick_count1=0;
    tick_count2=0;
    Stop();
    NVIC_DIS0_R = 1 << 23;     // 9) disable interrupt 23 in NVIC
    TIMER2_CTL_R = 0x00000000; // 10) disable timer2A
    SongIndex = 0;
    SongIndex2 = 0;
}

void ChangeTempo(void)
{
    if (tempo == 1)
    {
        tempo = 2;
    }
    else
    {
        tempo = 1;
    }
}
