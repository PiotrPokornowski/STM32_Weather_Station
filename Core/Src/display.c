#include "display.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "LCD_HD44780.h"

extern menu_t *currentMenu;
extern int markerRow;

#define LINE_LENGTH 20
char lastLine1[LINE_LENGTH + 1] = {0};
char lastLine2[LINE_LENGTH + 1] = {0};

uint8_t upArrow[8] = {0x0A, 0x0A, 0x1F, 0x1F, 0x0A, 0x0A, 0x00, 0x00};
uint8_t downArrow[8] = {0x00, 0x00, 0x0A, 0x0A, 0x1F, 0x1F, 0x0A, 0x0A};

void initCustomChars()
{
    LCD_DefChar(0, upArrow);
    LCD_DefChar(1, downArrow);
}

void navigateBack()
{
    if (currentMenu->parent)
    {
        markerRow = 1;
        currentMenu = currentMenu->parent;
    }
}

void selectMenu()
{
    if (currentMenu->child)
    {
        markerRow = 1;
        currentMenu = currentMenu->child;
    }
    else if (currentMenu->menuFunction)
    {
        currentMenu->menuFunction();
    }
}

void updateDisplay()
{
    char line1[LINE_LENGTH + 1] = {0};
    char line2[LINE_LENGTH + 1] = {0};

    menu_t *displayMenu = currentMenu;

    if (markerRow == 1)
    {
        snprintf(line1, LINE_LENGTH, ">%-18s", displayMenu->name);
        if (displayMenu->next != NULL)
            snprintf(line2, LINE_LENGTH, " %-18s", displayMenu->next->name);
        else
            snprintf(line2, LINE_LENGTH, " %-18s", "");
    }
    else
    {
        snprintf(line1, LINE_LENGTH, " %-18s", displayMenu->prev->name);
        snprintf(line2, LINE_LENGTH, ">%-18s", displayMenu->name);
    }

    if (strcmp(lastLine1, line1) != 0)
    {
        printToDisplay(0, 0, line1);
        strncpy(lastLine1, line1, LINE_LENGTH);
    }

    if (strcmp(lastLine2, line2) != 0)
    {
        printToDisplay(1, 0, line2);
        strncpy(lastLine2, line2, LINE_LENGTH);
    }
}

void printToDisplay(uint8_t line, uint8_t col, const char *text)
{
    LCD_Locate(col, line);
    LCD_String(text);
}
