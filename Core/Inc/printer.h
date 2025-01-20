#ifndef PRINTER_H
#define PRINTER_H

#include <stdio.h>
#include <string.h>
#include "stm32f4xx_hal.h"

#define INIT_PRINTER {0x1B, 0x40}    // ESC @ (Printer init)
#define LINE_FEED {0x0A}             // Line feed (LF)
#define CUT_PAPER {0x1D, 0x56, 0x00} // ESC + V + 0 (cut paper)

typedef struct
{
    UART_HandleTypeDef *uart;
} PRINTER_HandleTypedef;

void PRINTER_Init(PRINTER_HandleTypedef *dev);
void PRINTER_Print(PRINTER_HandleTypedef *dev, const char *text);

#endif // PRINTER_H
