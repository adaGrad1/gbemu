#ifndef MMU_H
#define MMU_H
#include "main.h"

typedef struct MBCT mbc_t;

typedef struct MMU {
    uint8_t (*mbc_fn)(gb_t*, uint16_t, uint8_t, uint8_t);
    mbc_t* mbc;
} mmu_t;

uint8_t get_mem(gb_t* s, uint16_t addr);
void set_mem(gb_t* s, uint16_t addr, uint8_t value);
void update_timer(gb_t* s, uint16_t cycles_passed);
void mmu_init(gb_t* s, char* fpath);
void save_persistent(gb_t* s, char* fpath);

#endif