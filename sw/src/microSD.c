#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/integer.h"
#include "../inc/diskio.h"
#include "../inc/SPI.h"
#include "../inc/ST7735.h"
#include "../inc/ff.h"

static FATFS g_sFatFs;
FIL Handle, Handle2;
FRESULT MountFresult;
FRESULT Fresult;
unsigned char buffer[512];
FATFS FS;           

static FIL bmpFile;        // Static prevents Stack Overflow
static uint8_t bmpHeader[54]; 

// lives = index (0..9), string = BMP filename
static const char * const P1LifeScreens[10] = {
    "p11.BMP",  // 0 lives
    "p12.BMP",  // 1 life
    "p13.BMP",
    "p14.BMP",
    "p15.BMP",
    "p16.BMP",
    "p17.BMP",
    "p18.BMP",
    "p19.BMP",
    "p110.BMP"   // 9 lives
};

static const char * const P2LifeScreens[10] = {
    "p21.BMP",
    "p22.BMP",
    "p23.BMP",
    "p24.BMP",
    "p25.BMP",
    "p26.BMP",
    "p27.BMP",
    "p28.BMP",
    "p29.BMP",
    "p210.BMP"
};


// Other “static” screens
static const char * const StartScreenBMP = "START.BMP";
static const char * const WinScreenBMP   = "WIN.BMP";
static const char * const LoseScreenBMP  = "LOSE.BMP";


// --- THE STREAMING FUNCTION ---
void LCD_DrawBMP(const char *filename, uint8_t x_pos, uint8_t y_pos) {
    FRESULT res;
    UINT bytesRead;
    static uint8_t rowBuf[512];   // Large enough for one BMP row (<= 160*3 + padding)

    // 1. Open file
    res = f_open(&bmpFile, filename, FA_READ);
    if (res != FR_OK) {
        ST7735_FillScreen(0);
        ST7735_DrawString(0, 0, "Open Err:", ST7735_Color565(255,0,0));
        ST7735_SetCursor(10, 0);
        ST7735_OutUDec(res);
        return;
    }

    // 2. Read header (54 bytes)
    res = f_read(&bmpFile, bmpHeader, 54, &bytesRead);
    if (res != FR_OK || bytesRead != 54) {
        f_close(&bmpFile);
        return;
    }

    // 3. Basic BMP validation
    if (bmpHeader[0] != 'B' || bmpHeader[1] != 'M') {
        ST7735_FillScreen(0);
        ST7735_DrawString(0, 0, "Not BMP", ST7735_Color565(255,0,0));
        f_close(&bmpFile);
        return;
    }

    // 32-bit little-endian width and height
    int32_t width  =  (int32_t)bmpHeader[18]
                    | ((int32_t)bmpHeader[19] << 8)
                    | ((int32_t)bmpHeader[20] << 16)
                    | ((int32_t)bmpHeader[21] << 24);

    int32_t height =  (int32_t)bmpHeader[22]
                    | ((int32_t)bmpHeader[23] << 8)
                    | ((int32_t)bmpHeader[24] << 16)
                    | ((int32_t)bmpHeader[25] << 24);

    uint16_t bpp = bmpHeader[28] | (bmpHeader[29] << 8);
    uint32_t compression =  (uint32_t)bmpHeader[30]
                          | ((uint32_t)bmpHeader[31] << 8)
                          | ((uint32_t)bmpHeader[32] << 16)
                          | ((uint32_t)bmpHeader[33] << 24);

    if (bpp != 24 || compression != 0) {
        ST7735_FillScreen(0);
        ST7735_DrawString(0, 0, "Bad BMP fmt", ST7735_Color565(255,0,0));
        f_close(&bmpFile);
        return;
    }

    // Pixel data start offset
    uint32_t dataOffset =  (uint32_t)bmpHeader[10]
                         | ((uint32_t)bmpHeader[11] << 8)
                         | ((uint32_t)bmpHeader[12] << 16)
                         | ((uint32_t)bmpHeader[13] << 24);

    // Seek to pixel data
    res = f_lseek(&bmpFile, dataOffset);
    if (res != FR_OK) {
        f_close(&bmpFile);
        return;
    }

    // Handle top-down vs bottom-up
    int32_t absHeight = height;
    uint8_t bottomUp = 1;
    if (height < 0) {
        absHeight = -height;
        bottomUp = 0;   // top-down
    }

    uint32_t absWidth = (width < 0) ? -width : width;

    // Compute row size & padding per BMP spec
    uint32_t rowSizeBits = (uint32_t)bpp * absWidth;
    uint32_t rowSize = ((rowSizeBits + 31U) / 32U) * 4U;   // bytes in file per row (including padding)
    uint32_t bytesPerPixel = 3;
    uint32_t dataBytesPerRow = absWidth * bytesPerPixel;

    if (rowSize > sizeof(rowBuf)) {
        // Row is too big for our buffer
        ST7735_FillScreen(0);
        ST7735_DrawString(0, 0, "Row too wide", ST7735_Color565(255,0,0));
        f_close(&bmpFile);
        return;
    }

    // 4. Draw loop, one row at a time
    for (int32_t row = 0; row < absHeight; row++) {
        uint16_t y = bottomUp
            ? (uint16_t)(y_pos + (absHeight - 1 - row))
            : (uint16_t)(y_pos + row);

        // Read whole row (pixels + padding) into buffer
        res = f_read(&bmpFile, rowBuf, rowSize, &bytesRead);
        if (res != FR_OK || bytesRead != rowSize) {
            f_close(&bmpFile);
            return;
        }

        if (y >= 160) {
            // Off-screen vertically, but we already consumed the row data
            continue;
        }

        // Loop through pixels in this row
        uint32_t maxVisibleWidth = absWidth;
        if (x_pos + maxVisibleWidth > 128) {
            maxVisibleWidth = 128 - x_pos; // clip to LCD width
        }

        for (uint32_t col = 0; col < maxVisibleWidth; col++) {
            uint32_t idx = col * 3;

            // Your image looked blue before, so we treat buffer as RGB
            uint8_t r = rowBuf[idx + 0];
            uint8_t g = rowBuf[idx + 1];
            uint8_t b = rowBuf[idx + 2];

            uint16_t color =
                ((r & 0xF8) << 8) |
                ((g & 0xFC) << 3) |
                ( b >> 3);

            ST7735_DrawPixel((uint16_t)(x_pos + col), y, color);
        }
        // Any padding is already in rowBuf but ignored by idx loop; no extra seek/read needed.
    }

    f_close(&bmpFile);
}

// ------------------- NEW DEBUG FUNC -----------------


void SD_Init(void){
	DSTATUS s;
uint32_t tries = 0;

do {
    s = disk_initialize(0);
    tries++;

    // Display status
    ST7735_SetCursor(0,0);
    ST7735_DrawString(0, 0, "disk_init:", ST7735_Color565(255,255,255));
    ST7735_SetCursor(10, 0);
    ST7735_OutUDec(s);

    ST7735_SetCursor(0,1);
    ST7735_DrawString(0, 1, "tries:", ST7735_Color565(255,255,0));
    ST7735_SetCursor(7, 1);
    ST7735_OutUDec(tries);

    // 10�20 ms delay so card settles
    for(volatile int d = 0; d < 1600000; d++){}

} while(s != 0 && tries < 50);   // stop after 50 attempts

if(s != 0){
    ST7735_DrawString(0, 2, "SD INIT FAIL", ST7735_Color565(255,0,0));
    while(1); // halt
}

  MountFresult = f_mount(&g_sFatFs, "", 0);
  if(MountFresult){
    ST7735_DrawString(0, 0, "f_mount error", ST7735_Color565(0, 0, 255));
    while(1){};
  }
	 f_mount(&FS, "0:", 1);
}




//----------------------- LCD Output --------------------------
void LCD_Start(void) {
    ST7735_FillScreen(0);
    LCD_DrawBMP(StartScreenBMP, 0, 0);
}

void LCD_Win(void) {
    ST7735_FillScreen(0);
    LCD_DrawBMP(WinScreenBMP, 0, 0);
}

void LCD_Lose(void) {
    ST7735_FillScreen(0);
    LCD_DrawBMP(LoseScreenBMP, 0, 0);
}

void LCD_ShowLives(uint8_t player, uint8_t lives) {
    if (lives == 0) {
        // optional: show lose screen or a special "0 life" screen
        LCD_Lose();
        return;
    }
    if (lives > 10) lives = 10;   // clamp just in case

    uint8_t idx = lives - 1;      // 10?0, 1?9

    const char *file;
    if (player == 1) {
        file = P1LifeScreens[idx];
    } else {
        file = P2LifeScreens[idx];
    }

    ST7735_FillScreen(0);
    LCD_DrawBMP(file, 0, 0);
}

