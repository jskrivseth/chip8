#ifndef CHIP8_SCREEN_H
#define CHIP8_SCREEN_H

#include <stdbool.h>
#include "config.h"

struct chip8_screen 
{
    bool pixels[CHIP8_DISPLAY_HEIGHT][CHIP8_DISPLAY_WIDTH];
};

void chip8_screen_xor(struct chip8_screen* screen, int x, int y);
bool chip8_screen_is_set(struct chip8_screen* screen, int x, int y);
bool chip8_screen_draw_sprite(struct chip8_screen* screen, int x, int y, const char* sprite, int num);
void chip8_screen_clear(struct chip8_screen* screen);

#endif