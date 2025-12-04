#include <stdint.h>
void LCD_DrawBMP(const char *filename, uint8_t x_pos, uint8_t y_pos);

void SD_Init(void);
void LCD_Start(void);
void LCD_Win(void);
void LCD_Lose(void);
void LCD_ShowLives(uint8_t player, uint8_t lives);
