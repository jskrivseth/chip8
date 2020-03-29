#include "chip8_screen.h"
#include <assert.h>
#include <stdio.h>
#include <memory.h>
#include <unistd.h>


static void chip8_screen_is_in_bounds(int x, int y) {
    assert(x >= 0 && x < CHIP8_DISPLAY_WIDTH && y >= 0 && y < CHIP8_DISPLAY_HEIGHT);
}

void chip8_screen_xor(struct chip8_screen* screen, int x, int y)
{
    chip8_screen_is_in_bounds(x, y);
    screen->pixels[y][x] ^= true;
}

bool chip8_screen_is_set(struct chip8_screen* screen, int x, int y)
{
    chip8_screen_is_in_bounds(x, y);
    return screen->pixels[y][x];
}

bool chip8_screen_draw_sprite(struct chip8_screen* screen, int x, int y, const char* sprite, int num) 
{
    bool pixel_collision = false;
    for (int ly = 0; ly < num; ly++) {
        char c = sprite[ly];
        for (int lx = 0; lx < 8; lx++) {
            if ((c & (0b10000000 >> lx)) == 0) 
            {
                continue;
            }
            
            int drawx = (lx+x) % CHIP8_DISPLAY_WIDTH;
            int drawy = (ly+y) % CHIP8_DISPLAY_HEIGHT;
            if (chip8_screen_is_set(screen, drawx, drawy)) {
                pixel_collision = true;
            }
            chip8_screen_xor(screen, drawx, drawy);
        }
    }
    return pixel_collision;
}

void chip8_screen_clear(struct chip8_screen* screen) 
{
    memset(screen->pixels, 0, sizeof(screen->pixels));
}
