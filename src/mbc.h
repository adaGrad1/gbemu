#ifndef MBC_H
#define MBC_H

#include "main.h"

enum MBC{
    NO_MBC = 0,
    MBC1 = 1,
    MBC2 = 5,
    MBC5 = 0x19
};

typedef struct MBC1 {
    uint8_t rambanks[8][0x2000];
    uint8_t current_rombank;
    uint8_t current_rambank;
    uint8_t bank_mode_select; 
} mbc1_t;

uint8_t no_mbc(gb_t* s, uint16_t addr, uint8_t value, uint8_t write);
uint8_t mbc1(gb_t* s, uint16_t addr, uint8_t value, uint8_t write);
void mbc_init(gb_t* s);

#endif