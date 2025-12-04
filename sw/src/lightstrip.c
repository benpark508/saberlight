#include <stdint.h>
#include "../inc/PLL.h"
#include "../inc/DMATimerWrite.h"
#include "../inc/tm4c123gh6pm.h"

// Blink the on-board LED.
#define GPIO_PORTE0 (*((volatile uint32_t *)0x40024004))
#define RED         0x02
#define BLUE        0x04
#define GREEN       0x08
const int32_t COLORWHEEL[8] = {RED, RED+GREEN, GREEN, GREEN+BLUE, BLUE, BLUE+RED, RED+GREEN+BLUE, 0};
#define NUMLEDS   30       // MODIFIED: number of LEDs in the string
#define SIZE 3*8*4*NUMLEDS // size of color encoding buffer (now 13824)
#define PINGPONGS 9         // MODIFIED: number of DMA ping-pong transfers needed ((SIZE/PINGPONGS/2)<=1024 and an integer) (13824/9/2 = 768)

// Each LED has three 8-bit color values stored in the order:
// [Green]  [Red]  [Blue]
// With each bit of each byte of each color of each LED stored
// in four bytes.  These four bytes encode a 0 as:
// {0x01, 0x00, 0x00, 0x00}
// and a 1 as:
// {0x01, 0x01, 0x00, 0x00}
// Note: These values are for LED string on GPIO pin 0.
// In other words, there are NUMLEDS LEDs per string of lights,
// 3 colors (green, red, blue) per LED, 8 bits per color, and
// 4 bytes per bit of color.


uint8_t Buffer[SIZE];      // the color encoding buffer
uint32_t Cursor;           // pointer into Buffer (0 to SIZE-1)
int animation_phase = 0;   // Global variable for animation state
int rainbow_phase = 0;

// --- Global Color Definitions ---
int Blade_R;
int Blade_G;
int Blade_B;



// delay function from sysctl.c
// which delays 3*ulCount cycles
//#ifdef __TI_COMPILER_VERSION__
  //Code Composer Studio Code
void Delay(uint32_t ulCount){
	while(ulCount--){
		__asm(" NOP");    // one no-op to keep the loop from being optimized away
	}
}



	
// ------------------------------------ GENERAL FUNCTIONS ------------------------------------
//------------sendreset------------
// Send a reset command to the WS2812 LED driver through PE0.
// This function uses a blind wait, so it takes at least 50
// usec to complete.
// Input: none
// Output: none
// Assumes: 80 MHz system clock, PE0 initialized
void sendreset(void){
  GPIO_PORTE0 = 0x00;           // hold data low               
	Delay(1333);                // delay ~4,000 cycles (50 usec)
}

//------------clearbuffer------------
// Clear the entire RAM buffer and restart the cursor to
// the beginning of the row of LEDs.  To actually update the
// physical LEDs, initiate a DMA transfer from 'Buffer' to
// GPIO_PORTE0 (0x40024004).
// Input: none
// Output: none
void clearbuffer(void){
  int i;
  Cursor = 0;                   // restart at the beginning of the buffer
  for(i=0; i<SIZE; i=i+4){
    Buffer[i] = 1;              // clear all color values for all LEDs by encoding 0's
    Buffer[i+1] = 0;
    Buffer[i+2] = 0;
    Buffer[i+3] = 0;
  }
}

//------------addcolor------------
// Private helper function that adds a color to the buffer.
// Configure the next LED with the desired color values and
// move to the next LED automatically.  This updates the RAM
// buffer and increments the cursor.  To actually update the
// physical LEDs, initiate a DMA transfer from 'Buffer' to
// GPIO_PORTE0 (0x40024004).  A color value of zero means
// that color is not illuminated.
// Input: red    8-bit red color value
//        green  8-bit green color value
//        blue   8-bit blue color value
// Output: none
void addcolor(uint8_t red, uint8_t green, uint8_t blue){
  int i;
  for(i=0x80; i>0x00; i=i>>1){   // store the green byte first
    if(green&i){
      Buffer[Cursor] = 1;
      Buffer[Cursor+1] = 1;
      Buffer[Cursor+2] = 0;
      Buffer[Cursor+3] = 0;
    } else{
      Buffer[Cursor] = 1;
      Buffer[Cursor+1] = 0;
      Buffer[Cursor+2] = 0;
      Buffer[Cursor+3] = 0;
    }
    Cursor = Cursor + 4;
  }
  for(i=0x80; i>0x00; i=i>>1){   // store the red byte second
    if(red&i){
      Buffer[Cursor] = 1;
      Buffer[Cursor+1] = 1;
      Buffer[Cursor+2] = 0;
      Buffer[Cursor+3] = 0;
    } else{
      Buffer[Cursor] = 1;
      Buffer[Cursor+1] = 0;
      Buffer[Cursor+2] = 0;
      Buffer[Cursor+3] = 0;
    }
    Cursor = Cursor + 4;
  }
  for(i=0x80; i>0x00; i=i>>1){   // store the blue byte third
    if(blue&i){
      Buffer[Cursor] = 1;
      Buffer[Cursor+1] = 1;
      Buffer[Cursor+2] = 0;
      Buffer[Cursor+3] = 0;
    } else{
      Buffer[Cursor] = 1;
      Buffer[Cursor+1] = 0;
      Buffer[Cursor+2] = 0;
      Buffer[Cursor+3] = 0;
    }
    Cursor = Cursor + 4;
  }
  if(Cursor >= SIZE){           // check if at the end of the buffer
    Cursor = 0;                 // wrap back to the beginning
  }
}

//------------setcursor------------
// Point the cursor at the desired LED index.  This LED will
// be the next one configured by the next call to addcolor().
// If the new cursor parameter is greater than the total
// number of LEDs, it will wrap around.  For example,
// setcursor(11) with a row of 10 LEDs will wrap around to the
// second (index=1) LED.
// Input: newCursor 8-bit new cursor index
// Output: none
void setcursor(uint8_t newCursor){
  newCursor = newCursor%(SIZE/3/8/4);
  Cursor = newCursor*3*8*4;
}

//------------lightstrip_init------------
void Lightstrip_Init(){
	  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R4;
                                // allow time for clock to stabilize
  while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R4) == 0){};
  GPIO_PORTE_DIR_R |= 0x01;     // make PE0 out
  GPIO_PORTE_AFSEL_R &= ~0x01;  // disable alt funct on PE0
  GPIO_PORTE_DR8R_R |= 0x01;    // enable 8 mA drive on PE0
  GPIO_PORTE_DEN_R |= 0x01;     // enable digital I/O on PE0
                                // configure PE0 as GPIO
  GPIO_PORTE_PCTL_R = (GPIO_PORTE_PCTL_R&0xFFFFFFF0)+0x00000000;
  GPIO_PORTE_AMSEL_R &= ~0x01;  // disable analog functionality on PE0
  sendreset();
  DMA_Init(32, (uint32_t *)&GPIO_PORTE0); // initialize DMA channel 8 for Timer5A transfer, every 24 cycles (0.4 usec)
}

void Lightstrip_Reset(){
	sendreset();    // send a reset
  DMA_Transfer((uint8_t *)Buffer, SIZE/PINGPONGS, PINGPONGS);
}

// ------------------------------------ LIGHTSABER FUNCTIONS ------------------------------------

/* --- FUNCTION 1: Solid Color (Basic Test) --- */
void LED_Solid() {
    int i;
    for(i = 0; i < NUMLEDS; i++) {
				addcolor(Blade_R, Blade_G, Blade_B);
    }
}

/* --- FUNCTION 2: Moving Blade (The "Hum" Animation) --- */
void LED_Moving() {
    int i;
    // Increment phase to make it move. Wrap around at 10.
    animation_phase++;
    if(animation_phase > 5) animation_phase = 0;

    for(i = 0; i < NUMLEDS; i++) {
        // Create a pattern every 5 LEDs using the phase
        // (i + animation_phase) % 5 shifts the pattern by 1 LED every frame
        if((i + animation_phase) % 5 == 0) {
            addcolor(Blade_R, Blade_G, Blade_B);       // Bright spot
        } else {
            addcolor(Blade_R/5, Blade_G/5, Blade_B/5); // Dim background (simulates hum/glow)
        }
    }
}

/* --- FUNCTION 3: Block (Clash Effect) --- */
void LED_Block() {
    int i;
    int clash_center = NUMLEDS / 2;      
    int clash_radius = 4;
    int start = clash_center - clash_radius;
    int end   = clash_center + clash_radius;
    
    // 1. Animation Timer
    animation_phase++; 

    // 2. PULSE LOGIC (The "Dim and Go Back" effect)
    // We cycle through 2 states every 10 frames:
    // - FRAMES 0-4:  High Power (Normal Brightness)
    // - FRAMES 5-9:  Low Power  (Dimmed - simulates energy drain)
    
    int brightness_divider;
    int cycle = animation_phase % 10; // Loops 0 to 9

    if (cycle < 5) {
        brightness_divider = 1; // Full brightness (The "Go Back" part)
    } else {
        brightness_divider = 4; // 1/4th brightness (The "Dim" part)
    }

    for(i = 0; i < NUMLEDS; i++) {
        // --- CLASH CENTER (Strobing White/Yellow) ---
        if(i >= start && i <= end) {
            // Random-looking strobe for the sparks
            if ((i + animation_phase) % 3 == 0) {
                 addcolor(50, 50, 50); // White Spark
            } else if ((i + animation_phase) % 3 == 1) {
                 addcolor(0, 0, 255);   // Yellow Heat
            } else {
                 addcolor(0, 0, 0);       // Black Gap
            }
        } 
        // --- BACKGROUND BLADE (Pulsing Red/Green/Blue) ---
        else {
            // Apply the calculated brightness pulse
            addcolor(Blade_R / brightness_divider, 
                     Blade_G / brightness_divider, 
                     Blade_B / brightness_divider);
        }
    }
}

/* --- FUNCTION 4: Hit (Full Flash) --- */
void LED_Hit() {
    int i;
    for(i = 0; i < NUMLEDS; i++) {
        addcolor(40, 50, 0); // Flash Yellow/Orange
    }
		Lightstrip_Reset();
		Delay(1200000);
}

/* ------ Start: Power Up (Extends Blade) ------ */
void LED_Start() {
    int i;
    
    // Static variable to track the height of the blade.
    // Starts at 0 (fully retracted).
    static int blade_height = 0; 

    // 1. Grow logic
    // Only increase if we are not at the top (NUMLEDS) yet.
    static int slow_count = 0;
    slow_count++;
    
    // Adjust this '3' to change the speed (Higher = Slower)
    if (slow_count >= 3){ 
        slow_count = 0;
        if (blade_height < NUMLEDS){
            blade_height++;
        }
    }

    // 2. Draw the blade
    for(i = 0; i < NUMLEDS; i++) {
        if (i < blade_height) {
            // LED is inside the current blade height -> Turn ON (Blue/Red)
            addcolor(Blade_R/4, Blade_G/4, Blade_B/4); // Example: Blue Blade
        } else {
            // LED is outside (above) the current blade height -> OFF
            addcolor(0, 0, 0);
        }
    }
}

/* ------ Lose: Power Down (Retracts Blade) ------ */
/* ------ Lose: Retract and Stay Off ------ */
void LED_Off(void) {
    int i;
    
    // Static variable to track the height of the blade.
    // Starts at NUMLEDS (full blade).
    // Because it is 'static', it remembers the value between loops.
    static int blade_height = NUMLEDS; 

    // 1. Shrink logic
    // Only decrease if we are not at the bottom yet.
    static int slow_count = 0;
		slow_count ++;
		if (slow_count >= 3){
			slow_count = 0;
			if (blade_height > 0){
				blade_height --;
			}
		}
    // If blade_height is 0, we do nothing to it. It stays 0.

    // 2. Draw the blade
    for(i = 0; i < NUMLEDS; i++) {
        if (i < blade_height) {
            // LED is inside the current blade height -> RED
            addcolor(Blade_R, Blade_G, Blade_B); 
        } else {
            // LED is outside the current blade height -> OFF
            addcolor(0, 0, 0);
        }
    }
}

/* ------ Lose: Critical Power Failure (Persistent Glitch) ------ */
void LED_Lose(void) {
    int i;
    
    // Increment global animation timer to drive the chaos
    animation_phase++; 

    for(i = 0; i < NUMLEDS; i++) {
        
        // CHAOS MATH: Generate a pseudo-random number (0-99) for this pixel
        // We use prime numbers (13, 23) to make the noise look random and organic
        int chaos = (i * 13 + animation_phase * 23) % 100;
        
        // --- Determine the state of this pixel based on the 'chaos' value ---
        
        if (chaos > 92) {
            // 8% Chance: ARC FLASH (Pure White)
            // Simulates a high-voltage short circuit
            addcolor(255, 255, 255); 
        } 
        else if (chaos > 75) {
            // 17% Chance: DEAD ZONE (Off)
            // Simulates gaps in the magnetic containment
            addcolor(0, 0, 0); 
        } 
        else if (chaos > 50) {
            // 25% Chance: BROWNOUT (Very Dim)
            // The power is failing in this section
            addcolor(Blade_R / 10, Blade_G / 10, Blade_B / 10); 
        } 
        else {
            // 50% Chance: UNSTABLE (Half Brightness)
            // The remaining blade is weak and flickering
            addcolor(Blade_R / 3, Blade_G / 3, Blade_B / 3);
        }
    }
}

/* ------ Win: Rainbow Wave ------ */
void LED_Win(void) {
    int i;
    int pixel_hue;
    int r, g, b;

    // 1. Move the animation frame forward
    // Change "+ 5" to a larger number to make it scroll faster
    // Change to a smaller number to scroll slower
    rainbow_phase = rainbow_phase + 5; 
    if (rainbow_phase > 255) {
        rainbow_phase = 0;
    }

    for (i = 0; i < NUMLEDS; i++) {
        // 2. Calculate the "Hue" (color position) for this specific LED.
        //
        // (i * 10) -> This is the "Spread". 
        // A value of 10 means the color changes slightly for every single LED.
        // If you see "too much red", INCREASE this number (try 20).
        // If the rainbow repeats too often, DECREASE this number (try 5).
        pixel_hue = (rainbow_phase + (i * 10)) & 255; 

        // 3. Convert Hue (0-255) to RGB (The Wheel Logic)
        if (pixel_hue < 85) {
            // Red to Green
            r = 255 - pixel_hue * 3;
            g = pixel_hue * 3;
            b = 0;
        } 
        else if (pixel_hue < 170) {
            // Green to Blue
            pixel_hue -= 85;
            r = 0;
            g = 255 - pixel_hue * 3;
            b = pixel_hue * 3;
        } 
        else {
            // Blue to Red
            pixel_hue -= 170;
            r = pixel_hue * 3;
            g = 0;
            b = 255 - pixel_hue * 3;
        }

        // 4. Output with Brightness Control
        // We divide by 4 to keep it vibrant but not blindingly white/power hungry
        addcolor(r/4, g/4, b/4);
    }
}

/* --- FUNCTION: Damage (Flicker OFF/Black) --- */
void LED_Damaged(void) {
    int i;
    
    // Increment the global animation timer
    animation_phase++;
    
    // Modulo 2 creates a rapid Strobe: Even = OFF, Odd = ON
    // (Change % 2 to % 4 for a slower strobe)
    if (animation_phase % 2 == 0) {
        // Frame A: Power Cut (Black)
        for(i = 0; i < NUMLEDS; 	i++) {
            addcolor(0, 0, 0); 
        }
    } 
    else {
        // Frame B: Power Restore (Normal Color)
        for(i = 0; i < NUMLEDS; i++) {
            addcolor(Blade_R, Blade_G, Blade_B);
        }
    }
}
