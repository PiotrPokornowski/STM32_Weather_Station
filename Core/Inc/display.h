#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

typedef struct MenuStruct menu_t;

struct MenuStruct
{
    const char *name;
    menu_t *next;
    menu_t *prev;
    menu_t *child;
    menu_t *parent;
    void (*menuFunction)(void);
};

void navigateBack(void);
void selectMenu(void);
void updateDisplay(void);
void printToDisplay(uint8_t line, uint8_t col, const char *text);

#endif // DISPLAY_H
