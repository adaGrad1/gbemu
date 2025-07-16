#include <stdio.h>

#include "util.h"
#include "main.h"
#include "instr_helpers.h"
#include "ppu.h"

void write_absolute(ppu_t* p, uint8_t x, uint8_t y, uint8_t val){ // write to an absolute position in 256 x 256 space; applies scroll
    x -= p->viewport_x;
    y -= p->viewport_y;
    if ((0 <= x) && (x < WIDTH) && (0 <= y) && (y < HEIGHT)) {
        p->display[y][x] = val;
    }
}

uint8_t apply_palette(uint8_t idx, uint8_t palette){
    return (palette >> (idx*2)) & 0x3;
}

// draw 8x8 tile to currently active scanline of screen
uint8_t draw_8x8_tile(ppu_t* p, uint8_t x_topleft, uint8_t y_topleft, uint8_t src[16], uint8_t palette, uint8_t flip_h, uint8_t flip_v, uint8_t transparency){
    // this is the Y-position of the scanline within the tile
    uint8_t yi = (p->scanline+p->viewport_y) - y_topleft;
    if (yi >= 8) return 1; // wrong vertical
    for(uint8_t xi = 0; xi < 8; xi++){
        uint8_t u = yi;
        if(flip_v) u = 7 - yi;
        uint8_t v = xi;
        if(flip_h) v = 7 - xi;
        uint8_t src_tile_2bit = get_bit(src[2*u], 7-v) + 2*get_bit(src[2*u+1], 7-v);
        if (!transparency || (src_tile_2bit > 0)) write_absolute(p, xi+x_topleft, yi+y_topleft, apply_palette(src_tile_2bit, palette));
    }
    return 0;
}

void vblank(ppu_t* p){
    memset(p->display, 0, WIDTH * HEIGHT);
}

void update_ppu(ppu_t* p, gb_t* s){
    p->viewport_y = (s->ram[0xFF42]);
    p->viewport_x = (s->ram[0xFF43]);
    uint8_t lcdc = s->ram[0xFF40];

    s->ram[0xFF44] = p->scanline;
    s->ram[0xFF41] &= 0xFC;
    if (p->scanline == 143){
        s->ram[0xFF41] |= 0x01; // vblank
    } else {
        s->ram[0xFF41] |= 0x00; // hblank
    }
    uint8_t do_interrupt = 0x00;

    if(get_bit(s->ram[0xFF41], 6)){
        do_interrupt = s->ram[0xFF44] == s->ram[0xFF45];
        // if(do_interrupt) printf("doing scanline int!\n");
    } else if(get_bit(s->ram[0xFF41], 5)){
        printf("looking for an interrupt that will never ever trigger because my PPU runs once per scanline!!\n");
        do_interrupt = (s->ram[0xFF41] & 3) == 2;
    } else if(get_bit(s->ram[0xFF41], 4)){
        do_interrupt = (s->ram[0xFF41] & 3) == 1;
    } else if(get_bit(s->ram[0xFF41], 3)){
        do_interrupt = (s->ram[0xFF41] & 3) == 0;
    }
    if (do_interrupt){
        set_bit(s->ram[0xFF0F], 1, 1);
    }

    if (!(lcdc & 0x80)) return; // LCD off

    if((lcdc & 0x01)) { // draw BG

        uint16_t tile_map_start_addr;
        if (lcdc & 0x08) {
            tile_map_start_addr = 0x9C00;
        } else {
            tile_map_start_addr = 0x9800;
        }
        uint8_t y = ((uint8_t)(p->scanline+(p->viewport_y))) >> 3;
        for(int x = 0; x < 32; x++) {
            uint16_t tile_idx = s->ram[tile_map_start_addr+32*y+x];
            uint16_t tile_data_addr;
            if(lcdc & 0x10) {
                tile_data_addr = 0x8000 + (tile_idx * 16);
            } else {
                tile_data_addr = 0x9000 + ((int8_t)tile_idx) * 16;
            }
            draw_8x8_tile(p->display, x*8, y*8, &s->ram[tile_data_addr], s->ram[0xFF47], 0, 0, 0);
        }

        if (lcdc & 0x20) { // enable window
                // uint8_t x_topleft = s->ram[]
                // uint8_t y = ((uint8_t)(p->scanline+(*p->viewport_y))) >> 3;
                // for(int x = 0; x < 32; x++) {
                //     uint16_t tile_idx = s->ram[tile_map_start_addr+32*y+x];
                //     uint16_t tile_data_addr;
                //     if(lcdc & 0x10) {
                //         tile_data_addr = 0x8000 + (tile_idx * 16);
                //     } else {
                //         tile_data_addr = 0x9000 + ((int8_t)tile_idx) * 16;
                //     }
                //     draw_8x8_tile(p->display, x*8, y*8, &s->ram[tile_data_addr], s->ram[0xFF47]);
                // }

        }
    }

    if (lcdc & 0x02) { // draw objects
        p->viewport_x = 0;
        p->viewport_y = 0;
        uint8_t drawn_so_far = 0;
        uint8_t tall_sprites = get_bit(lcdc, 2);
        for(uint8_t obj_idx = 0; obj_idx < 40; obj_idx++){
            uint16_t start_addr = 0xFE00+obj_idx*4;
            uint8_t y_pos = s->ram[start_addr];
            uint8_t x_pos = s->ram[start_addr+1];
            uint8_t tile_idx = s->ram[start_addr+2]; // which tile from the tile bank to render?
            uint8_t flags = s->ram[start_addr+3];
            if(tall_sprites) tile_idx &= 0xFFFE;
            uint8_t flip_v = get_bit(flags, 6);
            uint8_t flip_h = get_bit(flags, 5);
            // TODO: 
            // support for 8x16 tiles
            // proper palettes
            uint8_t palette = s->ram[0xFF48+get_bit(flags, 4)];
            // priority
            if(flip_v && tall_sprites) { // flip top and bottom tile index
            drawn_so_far += !draw_8x8_tile(p, x_pos-8, y_pos-16, &s->ram[0x8000 + (tile_idx+1) * 16], palette, flip_h, flip_v, 1);
            drawn_so_far += !draw_8x8_tile(p, x_pos-8, y_pos-8, &s->ram[0x8000 + (tile_idx) * 16], palette, flip_h, flip_v, 1);

            } else {
            drawn_so_far += !draw_8x8_tile(p, x_pos-8, y_pos-16, &s->ram[0x8000 + (tile_idx) * 16], palette, flip_h, flip_v, 1);
            if(tall_sprites) drawn_so_far += !draw_8x8_tile(p, x_pos-8, y_pos-8, &s->ram[0x8000 + (tile_idx+1) * 16], palette, flip_h, flip_v, 1);
            }
            if (drawn_so_far == 10) break; // only ten objects per scanline!!
        }
    }
}