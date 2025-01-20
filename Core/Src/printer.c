#include "printer.h"

void PRINTER_Init(PRINTER_HandleTypedef *dev)
{
    unsigned char init_printer[] = INIT_PRINTER;
    HAL_UART_Transmit(dev->uart, init_printer, sizeof(init_printer), HAL_MAX_DELAY);
}

void PRINTER_Print(PRINTER_HandleTypedef *dev, const char *text)
{
    unsigned char line_feed[] = LINE_FEED;
    unsigned char cut_paper[] = CUT_PAPER;
    size_t text_len = strlen(text);

    PRINTER_Init(dev); // NEEDED TO CLEAR INTERNAL PRINTER BUFFER

    if (text_len > 0)
    {
        HAL_UART_Transmit(dev->uart, (uint8_t *)text, text_len, HAL_MAX_DELAY);
    }

    for (int i = 0; i < 5; i++)
    {
        HAL_UART_Transmit(dev->uart, line_feed, sizeof(line_feed), HAL_MAX_DELAY);
    }

    HAL_UART_Transmit(dev->uart, cut_paper, sizeof(cut_paper), HAL_MAX_DELAY);
}
