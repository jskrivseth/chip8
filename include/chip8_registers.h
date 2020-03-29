#ifndef CHIP8_REGISTERS_H
#define CHIP8_REGISTERS_H

#include "config.h"

struct chip8_registers
{
    unsigned char V[CHIP8_NUM_REGISTERS];  //general purpose registers
    unsigned short I;  //
    unsigned char DT;  //delay timer
    unsigned char ST; //sound timer
    unsigned short PC; //program counter
    unsigned char SP;  //stack pointer
};

#endif