#include <stdio.h>

#include "util.h"
#include "main.h"
#include "instr_helpers.h"
#include "ppu.h"

void set_tile(uint8_t screen[256][256], uint8_t x, uint8_t y, uint8_t src[16], uint8_t palette){
    for(int yi = 0; yi < 8; yi++){
        for(int xi = 0; xi < 8; xi++){
            uint8_t src_tile_2bit = 2*get_bit(src[2*yi], 7-xi) + get_bit(src[2*yi+1], 7-xi);
            screen[yi+y][xi+x] = src_tile_2bit; // TODO palette
        }
    }
}

void update_ppu(ppu_t* p, gb_t* s){
    uint8_t lcdc = s->ram[0xFF40];
    printf("LCDC: 0x%02X\n", lcdc);
    if (!(lcdc & 0x80)) return; // LCD off

    // make 256 x 256 screen, pre-scroll
    uint16_t tile_map_start_addr;

    if (lcdc & 0x08) {
        tile_map_start_addr = 0x9C00;
    } else {
        tile_map_start_addr = 0x9800;
    }

    // uint8_t overdrawn_screen[256][256] = {0};
    // BG -- get palette indices
    printf("First few VRAM bytes: 0x8000=%02X 0x8001=%02X 0x8002=%02X\n", 
           s->ram[0x8000], s->ram[0x8001], s->ram[0x8002]);
    printf("First few tilemap bytes: 0x9800=%02X 0x9801=%02X 0x9802=%02X\n", 
           s->ram[0x9800], s->ram[0x9801], s->ram[0x9802]);
    printf("Tilemap dump:\n");
    for(int y = 0; y < 32; y++) {
        for(int x = 0; x < 32; x++) {
            uint8_t tile_idx = s->ram[tile_map_start_addr+32*y+x];
            printf("%x ", tile_idx);
            uint16_t tile_data_addr = 0x8000 + (tile_idx * 16);
            set_tile(p->display, x*8, y*8, &s->ram[tile_data_addr], 0);
        }
        printf("\n");

    }

    // apply scroll and place in ppu struct
    // uint8_t viewport_y = s->ram[0xFF42];
    // uint8_t viewport_x = s->ram[0xFF43];
}