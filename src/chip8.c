#include "chip8.h"
#include <memory.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <assert.h>

const char chip8_default_character_set[] = {
    0xf0, 0x90, 0x90, 0x90, 0xf0, //0
    0x20, 0x60, 0x20, 0x20, 0x70, //1
    0xf0, 0x10, 0xf0, 0x80, 0xf0, //2
    0xf0, 0x10, 0xf0, 0x10, 0xf0, //3
    0x90, 0x90, 0xf0, 0x10, 0x10, //4
    0xf0, 0x80, 0xf0, 0x10, 0xf0, //5
    0xf0, 0x80, 0xf0, 0x90, 0xf0, //6
    0xf0, 0x10, 0x20, 0x40, 0x40, //7
    0xf0, 0x90, 0xf0, 0x90, 0xf0, //8
    0xf0, 0x90, 0xf0, 0x10, 0xf0, //9
    0xf0, 0x90, 0xf0, 0x90, 0x90, //A
    0xe0, 0x90, 0xe0, 0x90, 0xe0, //B
    0xf0, 0x80, 0x80, 0x80, 0xf0, //C
    0xe0, 0x90, 0x90, 0x90, 0xe0, //D
    0xf0, 0x80, 0xf0, 0x80, 0xf0, //E
    0xf0, 0x80, 0xf0, 0x80, 0x80, //F
};

void chip8_init(struct chip8 *chip8)
{
    memset(chip8, 0, sizeof(struct chip8));
    memcpy(&chip8->memory.memory, chip8_default_character_set, sizeof(chip8_default_character_set));
}

void chip8_exec(struct chip8 *chip8, unsigned short opcode)
{
    switch (opcode)
    {
    case 0x00E0: //clear the screen
        chip8_screen_clear(&chip8->screen);
        break;
    case 0x00EE: //return from subroutine
        chip8->registers.PC = chip8_stack_pop(chip8);
        break;
    default:
        chip8_execute_extended(chip8, opcode);
    }
}

static void chip8_execute_extended(struct chip8 *chip8, unsigned short opcode)
{
    unsigned short nnn = opcode & 0x0fff;
    unsigned char x = (opcode >> 8) & 0x000f;
    unsigned char y = (opcode >> 4) & 0x000f;
    unsigned char n = opcode & 0x000f;
    unsigned char kk = opcode & 0x00ff;
    switch (opcode & 0xf000)
    {
    //JUMP to nnn
    case 0x1000:
    {
        chip8->registers.PC = nnn;
    }
    break;
    //CALL at nnn
    case 0x2000:
    {
        chip8_stack_push(chip8, chip8->registers.PC);
        chip8->registers.PC = nnn;
    }
    break;
    //SKIP if Vx == kk
    case 0x3000:
    {
        if (chip8->registers.V[x] == kk)
        {
            chip8->registers.PC += 2;
        }
    }
    break;
    //SKIP if Vx != kk
    case 0x4000:
    {
        if (chip8->registers.V[x] != kk)
        {
            chip8->registers.PC += 2;
        }
    }
    break;
    // SKIP if Vx == Vy
    case 0x5000:
    {
        if (chip8->registers.V[x] == chip8->registers.V[y])
        {
            chip8->registers.PC += 2;
        }
    }
    break;
    case 0x6000:
    {
        chip8->registers.V[x] = kk;
    }
    break;
    case 0x7000:
    {
        chip8->registers.V[x] += kk;
    }
    break;
    case 0x8000:
    {
        chip8_exec_0x8000(chip8, opcode);
    }
    break;
    case 0x9000:
    {
        if (chip8->registers.V[x] != chip8->registers.V[y])
        {
            chip8->registers.PC += 2;
        }
    }
    break;
    case 0xA000:
    {
        chip8->registers.I = nnn;
    }
    break;
    case 0xB000:
    {
        chip8->registers.PC = nnn + chip8->registers.V[0x00];
    }
    break;
    case 0xC000:
    {
        int time = (int)clock();
        srand(time);
        chip8->registers.V[x] = (rand() % 255) & kk;
    }
    break;
    case 0xD000:
    {
        const char *sprite = (const char *)&chip8->memory.memory[chip8->registers.I];
        chip8->registers.V[0x0f] = chip8_screen_draw_sprite(
            &chip8->screen,
            chip8->registers.V[x],
            chip8->registers.V[y],
            sprite,
            n);
    }
    break;
    //keyboard events
    case 0xE000:
    {
        switch (opcode & 0x00ff)
        {
        case 0x9e:
            if (chip8_keyboard_is_down(&chip8->keyboard, chip8->registers.V[x]))
            {
                chip8->registers.PC += 2;
            }
            break;
        case 0xa1:
            if (!chip8_keyboard_is_down(&chip8->keyboard, chip8->registers.V[x]))
            {
                chip8->registers.PC += 2;
            }
            break;
        }
    }
    break;
    //timers
    case 0xF000:
        chip8_exec_0xF000(chip8, opcode);
        break;
    }
}

void chip8_exec_0xF000(struct chip8 *chip8, unsigned short opcode)
{
    unsigned char x = (opcode >> 8) & 0x000f;
    unsigned char y = (opcode >> 4) & 0x000f;
    switch (opcode & 0x00ff)
    {
        //fx07 - LD Vx, DT. Set Vx to dt value
        case 0x07:
        {
            chip8->registers.V[x] = chip8->registers.DT;
        }

        break;
        case 0x0a:
        {
            char key = chip8_keyboard_wait(&chip8->keyboard);
            chip8->registers.V[x] = key;
        }
        break;
        // Fx15 - LD DT, Vx - set delay timer to Vx
        case 0x15:
            chip8->registers.DT = chip8->registers.V[x];
            break;
        case 0x18:
            chip8->registers.ST = chip8->registers.V[x];
            break;
        case 0x1E:
            chip8->registers.I += chip8->registers.V[x];
            break;
        case 0x29:
            chip8->registers.I = chip8->registers.V[x] * CHIP8_DEFAULT_SPRITE_HEIGHT;
            break;
            // fx33 - LD B, Vx
        case 0x33:
        {
            unsigned char a = chip8->registers.V[x] / 100;
            unsigned char b = chip8->registers.V[x] / 10 % 10;
            unsigned char c = chip8->registers.V[x] % 10;
            chip8_memory_set(&chip8->memory, chip8->registers.I, a);
            chip8_memory_set(&chip8->memory, chip8->registers.I + 1, b);
            chip8_memory_set(&chip8->memory, chip8->registers.I + 2, c);
        }
        break;
        case 0x55:
        {
            for (int i = 0; i <= x; i++)
            {
                chip8_memory_set(&chip8->memory, chip8->registers.I + i, chip8->registers.V[i]);
            }
        }
        break;
        case 0x65:
        {
            for (int i = 0; i <= x; i++)
            {
                chip8->registers.V[i] = chip8_memory_get(&chip8->memory, chip8->registers.I + i);
            }
            break;
        }
    }
}

void chip8_exec_0x8000(struct chip8 *chip8, unsigned short opcode)
{
    unsigned char x = (opcode >> 8) & 0x000f;
    unsigned char y = (opcode >> 4) & 0x000f;
    switch (opcode & 0x000f)
    {
        case 0x00:
            chip8->registers.V[x] = chip8->registers.V[y];
            break;
        case 0x01:
            chip8->registers.V[x] |= chip8->registers.V[y];
            break;
        case 0x02:
            chip8->registers.V[x] &= chip8->registers.V[y];
            break;
        case 0x03:
            chip8->registers.V[x] ^= chip8->registers.V[y];
            break;
            // 8xy4 - ADD Vx, Vy
        case 0x04:
        {
            unsigned short sum = chip8->registers.V[x] + chip8->registers.V[y];
            chip8->registers.V[0x0f] = sum > 0xff;
            chip8->registers.V[x] = sum;
        }
        break;
            //8xy5 - SUB Vx, Vy
        case 0x05:
        {
            chip8->registers.V[0x0f] = false;
            if (chip8->registers.V[x] > chip8->registers.V[y])
            {
                chip8->registers.V[0x0f] = true;
            }
            chip8->registers.V[x] -= chip8->registers.V[y];
        }
        break;
            //8xy6 - SHR Vx {, Vy}
        case 0x06:
        {
            chip8->registers.V[0x0f] = chip8->registers.V[x] & 0b00000001;
            chip8->registers.V[x] /= 2;
        }
        break;
            //8xy7 - SUBN Vx, Vy
        case 0x07:
        {
            chip8->registers.V[0x0f] = chip8->registers.V[y] > chip8->registers.V[x];
            chip8->registers.V[x] = chip8->registers.V[y] - chip8->registers.V[x];
        }
        break;
            //8xyE - SHL Vx {, Vy}
        case 0x0E:
        {
            chip8->registers.V[0x0f] = chip8->registers.V[x] & 0b10000000;
            chip8->registers.V[x] *= 2;
        }
        break;
    }
}

void chip8_load(struct chip8 *chip8, const char *buf, size_t size)
{
    assert(size + CHIP8_LOAD_ADDRESS < CHIP8_MEMORY_SIZE);
    memcpy(&chip8->memory.memory[CHIP8_LOAD_ADDRESS], buf, size);
    chip8->registers.PC = CHIP8_LOAD_ADDRESS;
}