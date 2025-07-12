#include <stdio.h>

#include "util.h"
#include "main.h"
#include "instr_helpers.h"
#include "ppu.h"

void set_tile(ppu_t* p, uint8_t x, uint8_t y, uint8_t src[16], uint8_t palette){
    int yi = p->scanline % 8;
    for(int xi = 0; xi < 8; xi++){
        uint8_t src_tile_2bit = 2*get_bit(src[2*yi], 7-xi) + get_bit(src[2*yi+1], 7-xi);
        p->display[yi+y][xi+x] = (palette >> src_tile_2bit*2) & 0x3;
    }
}

void vblank(ppu_t* p){
    memset(p->display, 0, WIDTH * HEIGHT);
}

void update_ppu(ppu_t* p, gb_t* s){
    uint8_t lcdc = s->ram[0xFF40];
    if (!(lcdc & 0x80)) return; // LCD off

    if((lcdc & 0x01)) {

        uint16_t tile_map_start_addr;
        if (lcdc & 0x08) {
            tile_map_start_addr = 0x9C00;
        } else {
            tile_map_start_addr = 0x9800;
        }
        int y = p->scanline >> 3;
        for(int x = 0; x < 32; x++) {
            uint8_t tile_idx = s->ram[tile_map_start_addr+32*y+x];
            uint16_t tile_data_addr = 0x8000 + (tile_idx * 16);
            set_tile(p->display, x*8, y*8, &s->ram[tile_data_addr], s->ram[0xFF47]);
        }
    }
    // apply scroll and place in ppu struct
    // uint8_t viewport_y = s->ram[0xFF42];
    // uint8_t viewport_x = s->ram[0xFF43];
}