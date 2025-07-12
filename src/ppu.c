#include <stdio.h>

#include "util.h"
#include "main.h"
#include "instr_helpers.h"
#include "ppu.h"

void write_absolute(ppu_t* p, uint8_t x, uint8_t y, uint8_t val){ // write to an absolute position in 256 x 256 space; applies scroll
    x -= *p->viewport_x;
    y -= *p->viewport_y;
    if ((0 <= x) && (x < WIDTH) && (0 <= y) && (y < HEIGHT)) {
        p->display[y][x] = val;
    }
}

uint8_t apply_palette(uint8_t idx, uint8_t palette){
    return (palette >> idx*2) & 0x3;
}

// draw 8x8 tile to currently active scanline of screen
uint8_t draw_8x8_tile(ppu_t* p, uint8_t x_topleft, uint8_t y_topleft, uint8_t src[16], uint8_t palette){
    // this is the Y-position of the scanline within the tile
    uint8_t yi = (p->scanline+*p->viewport_y) - y_topleft;
    if (yi >= 8) return 1; // wrong vertical
    for(uint8_t xi = 0; xi < 8; xi++){
        uint8_t src_tile_2bit = 2*get_bit(src[2*yi], 7-xi) + get_bit(src[2*yi+1], 7-xi);
        write_absolute(p, xi+x_topleft, yi+y_topleft, apply_palette(src_tile_2bit, palette));
    }
    return 0;
}

void vblank(ppu_t* p){
    memset(p->display, 0, WIDTH * HEIGHT);
}

void update_ppu(ppu_t* p, gb_t* s){
    p->viewport_y = &(s->ram[0xFF42]);
    p->viewport_x = &(s->ram[0xFF43]);
    uint8_t lcdc = s->ram[0xFF40];

    if (!(lcdc & 0x80)) return; // LCD off

    if((lcdc & 0x01)) { // draw BG

        uint16_t tile_map_start_addr;
        if (lcdc & 0x08) {
            tile_map_start_addr = 0x9C00;
        } else {
            tile_map_start_addr = 0x9800;
        }
        uint8_t y = ((uint8_t)(p->scanline+(*p->viewport_y))) >> 3;
        for(int x = 0; x < 32; x++) {
            uint8_t tile_idx = s->ram[tile_map_start_addr+32*y+x];
            uint16_t tile_data_addr = 0x8000 + (tile_idx * 16);
            draw_8x8_tile(p->display, x*8, y*8, &s->ram[tile_data_addr], s->ram[0xFF47]);
        }
    }

    if (lcdc & 0x02) { // draw objects
        uint8_t drawn_so_far = 0;
        for(uint8_t obj_idx = 0; obj_idx < 40; obj_idx++){
            uint16_t start_addr = 0xFE00+obj_idx*4;
            uint8_t y_pos = s->ram[start_addr];
            uint8_t x_pos = s->ram[start_addr+1];
            uint16_t tile_addr = 0x8000 + s->ram[start_addr+2] * 16; // 16 bytes per tile!
            uint8_t *tile_data = &(s->ram[tile_addr]);
            // TODO: 
            // support for 8x16 tiles
            // X/Y flip
            // proper palettes
            // priority
            if (!draw_8x8_tile(p, x_pos-8, y_pos-16, tile_data, 0xE4)){
                drawn_so_far++;
                if (drawn_so_far == 10) break; // only ten objects per scanline!!
            }
        }
    }
}