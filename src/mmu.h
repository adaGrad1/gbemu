#ifndef MMU_H
#define MMU_H
#include "main.h"

typedef struct MMU {
    uint8_t (*mbc_fn)(gb_t*, uint16_t, uint8_t, uint8_t);
    void* mbc;
} mmu_t;

uint8_t get_mem(gb_t* s, uint16_t addr);
void set_mem(gb_t* s, uint16_t addr, uint8_t value);
void update_timer(gb_t* s, uint16_t cycles_passed);
void mmu_init(gb_t* s);

#endif