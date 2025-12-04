// Lab7.c
// Runs on TM4C123
// Fall 2025

#include <stdint.h>
#include "mailbox.h"
#include "../inc/diskio.h"
#include "../inc/ff.h"
#include "../inc/tm4c123gh6pm.h"
#include "../inc/PLL.h"
#include "../inc/SysTick.h"
#include "../inc/GPIO_HAL.h"
#include "../inc/Unified_Port_Init.h"
#include "../inc/I2C3.h"
#include "../inc/mpu6500.h"
#include "../inc/MCP4821.h"
#include "../inc/cap1208.h"
#include "../inc/SPI.h"
#include "../inc/ST7735.h"
#include "../inc/Timer1A.h"
#include "music.h"

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void WaitForInterrupt(void);  // low power mode

//----- Prototypes of functions in startup.s  ----------------------
//
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical(void);     // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // Go into low power mode

raw_imu imu_raw;
processed_imu imu_proc;
volatile uint8_t printflag = 0;
uint8_t touch = 0;
int8_t count = 0;
extern volatile uint8_t go;

static FATFS g_sFatFs;
FIL Handle, Handle2;
FRESULT MountFresult;
FRESULT Fresult;
unsigned char buffer[512];
#define MAXBLOCKS 100
// Describe the error with text on the LCD and then stall.  If
// the error was caused by a mistake in configuring SSI0, then
// the LCD will probably not work.
void diskError(char *errtype, int32_t code, int32_t block)
{
  //  ST7735_DrawString(0, 0, "Error:", ST7735_Color565(255, 0, 0));
  //  ST7735_DrawString(7, 0, errtype, ST7735_Color565(255, 0, 0));
  //  ST7735_DrawString(0, 1, "Code:", ST7735_Color565(255, 0, 0));
  //  ST7735_SetCursor(6, 1);
  //  ST7735_SetTextColor(ST7735_Color565(255, 0, 0));
  //  ST7735_OutUDec(code);
  //  ST7735_DrawString(0, 2, "Block:", ST7735_Color565(255, 0, 0));
  //  ST7735_SetCursor(7, 2);
  //  ST7735_OutUDec(block);
  while (1)
  {
  };
}
// The simple unformatted test will destroy the formatting and
// erase everything on the SD card.
void SimpleUnformattedTest(void)
{
  DSTATUS result;
  uint16_t block;
  int i;
  uint32_t n;
  uint32_t errors = 0;
  // simple test of eDisk
  result = disk_initialize(0); // initialize disk

  // small pause to let the card settle (�10 ms at 80MHz)
  for (volatile int d = 0; d < 800000; d++)
  {
  }

  if (result)
    diskError("disk_initialize", result, 0);

  if (result)
    diskError("disk_initialize", result, 0);
  n = 1; // seed
  for (block = 0; block < MAXBLOCKS; block++)
  {
    for (i = 0; i < 512; i++)
    {
      n = (16807 * n) % 2147483647; // pseudo random sequence
      buffer[i] = 0xFF & n;
    }
    result = disk_write(0, buffer, block, 1);
    if (result)
      diskError("disk_write", result, block); // save to disk
  }
  n = 1; // reseed, start over to get the same sequence
  for (block = 0; block < MAXBLOCKS; block++)
  {
    result = disk_read(0, buffer, block, 1);
    if (result)
      diskError("disk_read ", result, block); // read from disk
    for (i = 0; i < 512; i++)
    {
      n = (16807 * n) % 2147483647; // pseudo random sequence
      if (buffer[i] != (0xFF & n))
      {
        errors = errors + 1;
      }
    }
  }
  //  ST7735_DrawString(0, 0, "Test done", ST7735_Color565(0, 255, 0));
  //  ST7735_DrawString(0, 1, "Mismatches:", ST7735_Color565(0, 255, 0));
  //  ST7735_SetCursor(12, 1);
  //  ST7735_SetTextColor(ST7735_Color565(0, 255, 0));
  //  ST7735_OutUDec(errors);
}
#define FILETESTSIZE 10000
void FileSystemTest(void)
{
  UINT successfulreads, successfulwrites;
  char c, d;
  int16_t x, y;
  int i;
  uint32_t n;
  c = 0;
  x = 0;
  y = 10;
  n = 1; // seed
  Fresult = f_open(&Handle2, "testFile.txt", FA_CREATE_ALWAYS | FA_WRITE);
  if (Fresult)
  {
    ST7735_DrawString(0, 0, "testFile error", ST7735_Color565(0, 0, 255));
    while (1)
    {
    };
  }
  else
  {
    for (i = 0; i < FILETESTSIZE; i++)
    {
      n = (16807 * n) % 2147483647; // pseudo random sequence
      c = ((n >> 24) % 10) + '0';   // random digit 0 to 9
      Fresult = f_write(&Handle2, &c, 1, &successfulwrites);
      if ((successfulwrites != 1) || (Fresult != FR_OK))
      {
        ST7735_DrawString(0, 0, "f_write error", ST7735_Color565(0, 0, 255));
        while (1)
        {
        };
      }
    }
    Fresult = f_close(&Handle2);
    if (Fresult)
    {
      ST7735_DrawString(0, 0, "file2 f_close error", ST7735_Color565(0, 0, 255));
      while (1)
      {
      };
    }
  }
  n = 1; // reseed, start over to get the same sequence
  Fresult = f_open(&Handle, "testFile.txt", FA_READ);
  if (Fresult == FR_OK)
  {
    ST7735_DrawString(0, 0, "Opened testFile.txt", ST7735_Color565(0, 0, 255));
    for (i = 0; i < FILETESTSIZE; i++)
    {
      n = (16807 * n) % 2147483647; // pseudo random sequence
      d = ((n >> 24) % 10) + '0';   // expected random digit 0 to 9
      Fresult = f_read(&Handle, &c, 1, &successfulreads);
      if ((successfulreads == 1) && (Fresult == FR_OK) && (c == d))
      {
        ST7735_DrawChar(x, y, c, ST7735_Color565(255, 255, 0), 0, 1);
        x = x + 6;
        if (x > 122)
        {
          x = 0; // start over on the next line
          y = y + 10;
        }
        if (y > 150)
        {
          y = 10; // the screen is full
        }
      }
      else
      {
        ST7735_DrawString(0, 0, "f_read error", ST7735_Color565(0, 0, 255));
        while (1)
        {
        };
      }
    }
  }
  else
  {
    ST7735_DrawString(0, 0, "Error testFile.txt (  )", ST7735_Color565(255, 0, 0));
    ST7735_SetCursor(20, 0);
    ST7735_SetTextColor(ST7735_Color565(255, 0, 0));
    ST7735_OutUDec((uint32_t)Fresult);
    while (1)
    {
    };
  }
  ST7735_DrawString(0, 0, "file test passed    ", ST7735_Color565(255, 255, 255));
  Fresult = f_close(&Handle);
  /*  Fresult = f_open(&Handle,"out.txt", FA_CREATE_ALWAYS|FA_WRITE);
    if(Fresult == FR_OK){
      ST7735_DrawString(0, 0, "Opened out.txt", ST7735_Color565(0, 0, 255));
      c = 'A';
      x = 0;
      y = 10;
      Fresult = f_write(&Handle, &c, 1, &successfulreads);
      ST7735_DrawChar(x, y, c, ST7735_Color565(255, 255, 0), 0, 1);
      while((c <= 'Z') && (Fresult == FR_OK)){
        x = x + 6;
        c = c + 1;
        if(x > 122){
          x = 0;                          // start over on the next line
          y = y + 10;
        }
        if(y > 150){
          break;                          // the screen is full
        }
        Fresult = f_write(&Handle, &c, 1, &successfulreads);
        ST7735_DrawChar(x, y, c, ST7735_Color565(255, 255, 0), 0, 1);
      }
    } else{
      ST7735_DrawString(0, 0, "Error out.txt (  )", ST7735_Color565(255, 0, 0));
      ST7735_SetCursor(15, 0);
      ST7735_SetTextColor(ST7735_Color565(255, 0, 0));
      ST7735_OutUDec((uint32_t)Fresult);
    }*/
}

const char inFilename[] = "test.txt"; // 8 characters or fewer
const char outFilename[] = "out.txt"; // 8 characters or fewer

void debug_serial(void)
{
  printflag = 1;
}

int main(void)
{
  DisableInterrupts();
  Unified_Port_Init();
  UINT successfulreads, successfulwrites;
  uint8_t c, x, y;
  PLL_Init(Bus80MHz); // bus clock at 80 MHz
  SysTick_Init();
  SPI_Init(200); // initialize SSI0 at 400 kHz
  CAP1208_Init();
  MPU6500_Init();
  long sr = StartCritical();
  MPU6500_calibrate(&imu_proc);
  EndCritical(sr);
  ST7735_InitR(INITR_GREENTAB);           // initialize LCD
  Timer1A_Init(debug_serial, 8000000, 2); // print every 100 ms
  Music_Init();

  /*
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
  FileSystemTest();                     // comment this out if file system works
  // open the file to be read
//  Fresult = f_open(&Handle, "Proc.axf", FA_READ);
//  if(Fresult != FR_OK){
//    ST7735_DrawString(0, 0, "Proc.axf f_open error", ST7735_Color565(0, 0, 255));
//    while(1){
//    }
//  }
  Fresult = f_open(&Handle, inFilename, FA_READ);
  if(Fresult == FR_OK){
    ST7735_DrawString(0, 0, "Opened ", ST7735_Color565(0, 255, 0));
    ST7735_DrawString(7, 0, (char *)inFilename, ST7735_Color565(0, 255, 0));
    // get a character in 'c' and the number of successful reads in 'successfulreads'
    Fresult = f_read(&Handle, &c, 1, &successfulreads);
    x = 0;                              // start in the first column
    y = 10;                             // start in the second row
    while((Fresult == FR_OK) && (successfulreads == 1) && (y <= 130)){
      if(c == '\n'){
        x = 0;                          // go to the first column (this seems implied)
        y = y + 10;                     // go to the next row
      } else if(c == '\r'){
        x = 0;                          // go to the first column
      } else{                           // the character is printable, so print it
        ST7735_DrawChar(x, y, c, ST7735_Color565(255, 255, 255), 0, 1);
        x = x + 6;                      // go to the next column
        if(x > 122){                    // reached the right edge of the screen
          x = 0;                        // go to the first column
          y = y + 10;                   // go to the next row
        }
      }
      // get the next character in 'c'
      Fresult = f_read(&Handle, &c, 1, &successfulreads);
    }
    // close the file
    Fresult = f_close(&Handle);
  } else{
    // print the error code
    ST7735_DrawString(0, 0, "Error          (  )", ST7735_Color565(255, 0, 0));
    ST7735_DrawString(6, 0, (char *)inFilename, ST7735_Color565(255, 0, 0));
    ST7735_SetCursor(16, 0);
    ST7735_SetTextColor(ST7735_Color565(255, 0, 0));
    ST7735_OutUDec((uint32_t)Fresult);
  }

  // open the file to be written
  // Options:
  // FA_CREATE_NEW    - Creates a new file, only if it does not already exist.  If file already exists, the function fails.
  // FA_CREATE_ALWAYS - Creates a new file, always.  If file already exists, it is over-written.
  // FA_OPEN_ALWAYS   - Opens a file, always.  If file does not exist, the function creates a file.
  // FA_OPEN_EXISTING - Opens a file, only if it exists.  If the file does not exist, the function fails.
  Fresult = f_open(&Handle, outFilename, FA_WRITE|FA_OPEN_ALWAYS);
  if(Fresult == FR_OK){
    ST7735_DrawString(0, 14, "Opened ", ST7735_Color565(0, 255, 0));
    ST7735_DrawString(7, 14, (char *)outFilename, ST7735_Color565(0, 255, 0));
    // jump to the end of the file
    Fresult = f_lseek(&Handle, Handle.fsize);
    // write a message and get the number of successful writes in 'successfulwrites'
    Fresult = f_write(&Handle, "Another successful write.\r\n", 27, &successfulwrites);
    if(Fresult == FR_OK){
      // print the number of successful writes
      // expect: third parameter of f_write()
      ST7735_DrawString(0, 15, "Writes:    @", ST7735_Color565(0, 255, 0));
      ST7735_SetCursor(8, 15);
      ST7735_SetTextColor(ST7735_Color565(255, 255, 255));
      ST7735_OutUDec((uint32_t)successfulwrites);
      ST7735_SetCursor(13, 15);
      // print the byte offset from the start of the file where the writes started
      // expect: (third parameter of f_write())*(number of times this program has been run before)
      ST7735_OutUDec((uint32_t)(Handle.fptr - successfulwrites));
    } else{
      // print the error code
      ST7735_DrawString(0, 15, "f_write() error (  )", ST7735_Color565(255, 0, 0));
      ST7735_SetCursor(17, 15);
      ST7735_SetTextColor(ST7735_Color565(255, 0, 0));
      ST7735_OutUDec((uint32_t)Fresult);
    }
    // close the file
    Fresult = f_close(&Handle);
  } else{
    // print the error code
    ST7735_DrawString(0, 14, "Error          (  )", ST7735_Color565(255, 0, 0));
    ST7735_DrawString(6, 14, (char *)outFilename, ST7735_Color565(255, 0, 0));
    ST7735_SetCursor(16, 14);
    ST7735_SetTextColor(ST7735_Color565(255, 0, 0));
    ST7735_OutUDec((uint32_t)Fresult);
  }

  SysTick_Wait10ms(10000); // wait 10 seconds to read the screen

  */
  EnableInterrupts();

  ST7735_FillScreen(ST7735_BLACK);
  ST7735_SetCursor(0, 0);
  ST7735_OutString("cap1208 demo");

  ST7735_SetCursor(0, 2); // Line 2
  ST7735_OutString("Delta: ");

  ST7735_SetCursor(0, 4); // Line 2
  ST7735_OutString("Touch: ");

  ST7735_SetCursor(0, 6); // Line 2
  ST7735_OutString("ax: ");

  ST7735_SetCursor(0, 8); // Line 4
  ST7735_OutString("ay: ");

  ST7735_SetCursor(0, 10); // Line 6
  ST7735_OutString("az: ");
  
  ST7735_SetCursor(0, 12);
  ST7735_OutString("Swing: ");

  while (1)
  {
    if (printflag)
    {
      printflag = 0;

      ST7735_SetCursor(7, 4);
      ST7735_OutString("NO");
      ST7735_OutString("   ");

      CAP1208_ReadCount(1, &count);

      MPU6500_getData(&imu_raw, &imu_proc);

      ST7735_SetCursor(7, 2);
      ST7735_OutSDec8(count);
      ST7735_OutString("   ");

      ST7735_SetCursor(7, 6);
      //ST7735_OutSDec16(imu_proc.accel_x_mg);
      ST7735_OutSDec16(imu_proc.gyro_x_mdps);
      ST7735_OutString("  ");
      ST7735_SetCursor(7, 8);
      //ST7735_OutSDec16(imu_proc.accel_y_mg);
      ST7735_OutSDec16(imu_proc.gyro_y_mdps);
      ST7735_OutString("  ");
      ST7735_SetCursor(7, 10);
      //ST7735_OutSDec16(imu_proc.accel_z_mg);
      ST7735_OutSDec16(imu_proc.gyro_z_mdps);
      ST7735_OutString("  ");
      
      ST7735_SetCursor(7, 12);
      if(MPU6500_DetectSwing(&imu_proc)){
        ST7735_OutString("YES");
        Music_Play();
      }
      else{
        ST7735_OutString("NO");
        Music_Stop();
      }
      ST7735_OutString("  ");
    }
  }
}
