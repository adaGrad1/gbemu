#ifndef PPU_H_
#define PPU_H_

#include <stdio.h>

#include "util.h"
#include "main.h"

#define WIDTH 256
#define HEIGHT 256

typedef struct ppu {
    uint8_t display[WIDTH][HEIGHT];
    uint8_t scanline;
} ppu_t;

void update_ppu(ppu_t* p, gb_t* s);

#endif