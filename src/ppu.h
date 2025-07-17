#ifndef PPU_H_
#define PPU_H_

#include <stdio.h>

#include "util.h"
#include "main.h"

#define WIDTH 160
#define HEIGHT 144

typedef struct ppu {
    uint8_t display[HEIGHT][WIDTH];
    uint8_t scanline;
    uint8_t viewport_x;
    uint8_t viewport_y;
} ppu_t;

void update_ppu(ppu_t* p, gb_t* s);
void update_ppu_flags(ppu_t* p, gb_t* s, uint16_t cycles_in_row);

#endif