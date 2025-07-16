#include "mbc.h"
#include "mmu.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#define min(a, b) (((a) < (b)) ? (a) : (b))

uint16_t n_rombanks(gb_t* s){
    return (2 << s->rom[0x148]);
}
uint8_t RAMBANK_MAP[] = {0, 0, 1, 4, 16, 8};
uint8_t n_rambanks(gb_t* s){
    return RAMBANK_MAP[s->rom[0x149]];
}

uint8_t no_mbc(gb_t* s, uint16_t addr, uint8_t value, uint8_t write) {
    if(write) {
        if((0x0000 <= addr) && (addr < 0x8000)) return 0;
        else s->ram[addr] = value;
    }
    else {
        return s->ram[addr];
    }
}

uint8_t mbc1(gb_t* s, uint16_t addr, uint8_t value, uint8_t write) {
    mbc1_t* mbc = (mbc1_t*)s->mmu->mbc;
    if(mbc == 0){
        printf("Init MBC-1\n");
        mbc = calloc(1, sizeof(mbc1_t));
        if(n_rambanks(s) == 0) {
            mbc->bank_mode_select = 1; // no RAM -- assume we are indexing upper bits of ROM.
        }
        s->mmu->mbc = mbc;
    }
    if(write) {
        if((0x0000 <= addr) && (addr < 0x2000)) 0;
        if((0x2000 <= addr) && (addr < 0x4000)) { // ROMBank switch
            if(value == 0) value = 1;
            mbc->current_rombank &= 0xE0;
            mbc->current_rombank |= (value & 0x1F);
            mbc->current_rombank = mbc->current_rombank % n_rombanks(s);
            printf("ROMBANK: %d\n", mbc->current_rombank);
            memcpy(s->ram+0x4000, s->rom+(0x4000*(mbc->current_rombank)), 0x4000);
        }
        if((0x4000 <= addr) && (addr < 0x6000)) { 
            uint8_t bank_no = value & 0x03;
            if (mbc->bank_mode_select) { // upper bits ROMBank switch
                printf("UPPER ROMBANK SWITCH\n");
                mbc->current_rombank &= 0x1F;
                mbc->current_rombank |= (value << 5);
                mbc->current_rombank = mbc->current_rombank % n_rombanks(s);
                memcpy(s->ram+0x4000, s->rom+(0x4000*(mbc->current_rombank)), 0x4000); 
            }
            else { // RAMBank switch
                printf("RAMBANK SWITCH\n");
                memcpy(mbc->rambanks[mbc->current_rambank], &(s->ram[0xA000]), 0x2000);
                mbc->current_rambank = bank_no;
                memcpy(s->ram+0xA000, mbc->rambanks[mbc->current_rambank], 0x2000);
            }
        }
        if((0x6000 <= addr) && (addr < 0x8000)) { // bank mode switch
            if(n_rambanks(s) > 0 && n_rombanks(s) > 32){ // we are even in a multi-bank situation
                mbc->bank_mode_select = value;
            }
        }
    } else {
        return s->ram[addr];
    }
}

void mbc_init(gb_t* s){
    switch(s->rom[0x0147]){
        case NO_MBC:
        case 8:
        case 9:
            s->mmu->mbc_fn = &no_mbc;
            break;
        case MBC1:
        case MBC1+1:
        case MBC1+2:
            printf("MBC1\n");
            s->mmu->mbc_fn = &mbc1;
            break;
        case MBC5:
        case MBC5+1:
        case MBC5+2:
        case MBC5+3:
        case MBC5+4:
        case MBC5+5:
            printf("MBC5\n");
            s->mmu->mbc_fn = &mbc1;
            break;
        default:
            printf("Error: Unknown membank %x!\n", s->ram[0x0147]);
            exit(0);
    }
}