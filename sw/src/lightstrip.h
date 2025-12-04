

#include <stdint.h>

// Configuration Constants
#define NUMLEDS 30
extern int Blade_R;
extern int Blade_G;
extern int Blade_B;
extern int animation_phase;
extern int rainbow_phase;


void Delay(uint32_t ulCount);

void Lightstrip_Init();
void Lightstrip_Reset();

void setcursor(uint8_t newCursor);
	
	
void LED_Solid(void);
void LED_Moving(void);
void LED_Block(void);
void LED_Hit(void);
void LED_Start(void);
void LED_Off(void);
void LED_Win(void);
void LED_Lose(void);
void LED_Damaged(void);