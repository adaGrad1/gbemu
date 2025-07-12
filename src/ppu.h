#ifndef PPU_H_
#define PPU_H_

#include <stdio.h>

#include "util.h"
#include "main.h"

typedef struct ppu {
    uint8_t display[256][256];
} ppu_t;

void update_ppu(ppu_t* p, gb_t* s);

#endif