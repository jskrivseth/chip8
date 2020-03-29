#include <stdio.h>
#include "SDL2/SDL.h"
#include "SDL2/SDL_audio.h"
#include "chip8.h"
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

const char keyboard_map[CHIP8_TOTAL_KEYS] = {
    SDLK_0, SDLK_1, SDLK_2,
    SDLK_3, SDLK_4, SDLK_5,
    SDLK_6, SDLK_7, SDLK_8,
    SDLK_9, SDLK_a, SDLK_b,
    SDLK_c, SDLK_d, SDLK_e,
    SDLK_f};

double lastRenderTime = 0;

double diffclock(clock_t clock1, clock_t clock2)
{
    double diffticks = clock2 - clock1;
    double diffms = (diffticks) / (CLOCKS_PER_SEC / 1000);
    return diffms;
}

void drawScreen(struct chip8_screen *screen, struct SDL_Renderer *renderer)
{

    SDL_SetRenderDrawColor(renderer, 32, 32, 32, 0);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 64, 160, 64, 96);
    for (int x = 0; x < CHIP8_DISPLAY_WIDTH; x++)
    {
        for (int y = 0; y < CHIP8_DISPLAY_HEIGHT; y++)
        {
            if (chip8_screen_is_set(screen, x, y))
            {
                SDL_Rect r;
                r.x = x * CHIP8_WINDOW_SCALER;
                r.y = y * CHIP8_WINDOW_SCALER;
                r.w = CHIP8_WINDOW_SCALER - 1;
                r.h = CHIP8_WINDOW_SCALER - 1;
                SDL_RenderFillRect(renderer, &r);
            }
        }
    }
    SDL_RenderPresent(renderer);
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("You must provide a file to load\n");
        return -1;
    }

    const char *filename = argv[1];
    FILE *f = fopen(filename, "rb");
    if (!f)
    {
        printf("Failed to open file\n");
        return -1;
    }
    fseek(f, 0, SEEK_END);
    long filesize = ftell(f);
    fseek(f, 0, SEEK_SET);
    char buf[filesize];
    fread(buf, filesize, 1, f);

    struct chip8 chip8;
    chip8_init(&chip8);
    chip8_load(&chip8, buf, filesize);
    chip8_keyboard_set_map(&chip8.keyboard, keyboard_map);

    double sixtyHertzTimer = 0;

    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window *window = SDL_CreateWindow(
        EMULATOR_WINDOW_TITLE,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        CHIP8_DISPLAY_WIDTH * CHIP8_WINDOW_SCALER,
        CHIP8_DISPLAY_HEIGHT * CHIP8_WINDOW_SCALER,
        SDL_WINDOW_SHOWN | SDL_WINDOW_INPUT_FOCUS);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_TARGETTEXTURE);

    while (1)
    {
        clock_t start = clock();
        SDL_Event event;
        
        if (SDL_PollEvent(&event) == 1)
        {
            switch (event.type)
            {
            case SDL_QUIT:
                goto out;
                break;

            case SDL_KEYDOWN:
            {
                char key = event.key.keysym.sym;
                int vkey = chip8_keyboard_map(&chip8.keyboard, key);
                if (vkey != -1)
                {
                    chip8_keyboard_down(&chip8.keyboard, vkey);
                }
            }
            break;

            case SDL_KEYUP:
            {
                char key = event.key.keysym.sym;
                int vkey = chip8_keyboard_map(&chip8.keyboard, key);
                if (vkey != -1)
                {
                    chip8_keyboard_up(&chip8.keyboard, vkey);
                }
            }
            break;
            }
        }
        SDL_PumpEvents();

        unsigned short opcode = chip8_memory_get_short(&chip8.memory, chip8.registers.PC);
        chip8.registers.PC += 2;
        chip8_exec(&chip8, opcode);

        //if we're due to fire a 60hz event, do it now
        if ((start - sixtyHertzTimer) >= (CLOCKS_PER_SEC / 600))
        {
            if (chip8.registers.DT > 0)
            {
                chip8.registers.DT -= 1;
                printf("DT : %d \n", chip8.registers.DT);
            }
            if (chip8.registers.ST > 0)
            {
                //todo implement sound
                chip8.registers.ST -= 1;
            }
            sixtyHertzTimer = clock();
        }

        drawScreen(&chip8.screen, renderer);

        clock_t stop = clock();
        lastRenderTime = diffclock(start, stop);
        float sleep_for = (lastRenderTime * 1000) / 120;
        //printf("Render took %f  %f  %f, sleep for: %f, clocks per: %d, clock: %ld  \n", lastRenderTime, (double)start, (double)stop, sleep_for, CLOCKS_PER_SEC, clock());
        SDL_Delay(floor(sleep_for));
    }
out:
    SDL_DestroyWindow(window);
    return 0;
}