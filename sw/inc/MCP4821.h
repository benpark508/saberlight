// MCP4821.h

#ifndef __MCP4821_H__
#define __MCP4821_H__
#include <stdint.h>

void DAC_Init(uint16_t data);
void DAC_Out(uint16_t code);
void DAC_Out_NB(uint16_t code);  

#endif