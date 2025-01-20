#include "encoder.h"
#include "display.h"
#include <stddef.h>
#include <usart.h>

extern menu_t *currentMenu;
extern uint8_t markerRow;

void Encoder_Clockwise(void)
{
    if (currentMenu->next != NULL)
    {
        currentMenu = currentMenu->next;
    }

    if (markerRow == 1)
    {
        markerRow = 2;
    }
    else
    {
        markerRow = 1;
    }
}

void Encoder_CounterClockwise(void)
{
    if (currentMenu->prev != NULL)
    {
        currentMenu = currentMenu->prev;
    }

    if (markerRow == 1)
    {
        markerRow = 2;
    }
    else
    {
        markerRow = 1;
    }
}