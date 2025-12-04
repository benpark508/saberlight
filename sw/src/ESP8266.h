// ESP8266.h
// Hardware abstraction for Custom PCB connection to ESP8266
// Handles Port C safety (JTAG protection) and Reset Logic.

#ifndef ESP8266_H
#define ESP8266_H

#include <stdint.h>

// Initializes Port C (PC4=Rx, PC5=Tx, PC6=RST, PC7=RDY)
// safely without breaking JTAG (PC0-PC3).
void ESP8266_Init(void);

// Performs a hard reset pulse on PC6 to reboot the ESP.
void ESP8266_Reset(void);

#endif // ESP8266_H