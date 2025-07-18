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
    mbc_t* mbc = s->mmu->mbc;
    if(write) {
        if((0x0000 <= addr) && (addr < 0x2000)) 0;
        if((0x2000 <= addr) && (addr < 0x4000)) { // ROMBank switch
            if(value == 0) value = 1;
            mbc->current_rombank &= 0xE0;
            mbc->current_rombank |= (value & 0x1F);
            mbc->current_rombank = mbc->current_rombank % n_rombanks(s);
            memcpy(s->ram+0x4000, s->rom+(0x4000*(mbc->current_rombank)), 0x4000);
        }
        if((0x4000 <= addr) && (addr < 0x6000)) { 
            uint8_t bank_no = value & 0x03;
            if (mbc->bank_mode_select) { // upper bits ROMBank switch
                mbc->current_rombank &= 0x1F;
                mbc->current_rombank |= (value << 5);
                mbc->current_rombank = mbc->current_rombank % n_rombanks(s);
                memcpy(s->ram+0x4000, s->rom+(0x4000*(mbc->current_rombank)), 0x4000); 
            }
            else { // RAMBank switch
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

uint8_t mbc3(gb_t* s, uint16_t addr, uint8_t value, uint8_t write) {
    mbc_t* mbc = s->mmu->mbc;
    printf("MCB3 access!\n");
    if (write) {
        if((0x2000 <= addr) && (addr < 0x4000)) { // ROMBank switch
            mbc->current_rombank = value % n_rombanks(s);
            memcpy(s->ram+0x4000, s->rom+(0x4000*(mbc->current_rombank)), 0x4000);
        }
        else if((0x4000 <= addr) && (addr < 0x6000)) { // RAMBank switch
            if(mbc->current_rambank < 0x08) memcpy(mbc->rambanks[mbc->current_rambank], &(s->ram[0xA000]), 0x2000);
            mbc->current_rambank = value;
            if(mbc->current_rambank < 0x08) memcpy(s->ram+0xA000, mbc->rambanks[mbc->current_rambank], 0x2000);
            else memset(s->ram+0xA000, 1, 0x2000);
        }
        printf("ROMBANK: %x RAMBANK: %x", mbc->current_rombank, mbc->current_rambank);
    } else {
        if(mbc->current_rambank >= 8) printf("RTC accessed: RAMBank=%x, returned %x\n", mbc->current_rambank, s->ram[addr]);
        return s->ram[addr];
    }
}



uint8_t mbc5(gb_t* s, uint16_t addr, uint8_t value, uint8_t write) {
    mbc_t* mbc = s->mmu->mbc;
    if(write) {
        if((0x0000 <= addr) && (addr < 0x2000)) mbc->ram_enabled=((value&0x0F)==0x0A);
        else if((0x2000 <= addr) && (addr < 0x3000)) { // ROMBank switch
            mbc->current_rombank &= 0xFF00;
            mbc->current_rombank |= (value & 0xFF);
            mbc->current_rombank = mbc->current_rombank % n_rombanks(s);
            memcpy(s->ram+0x4000, s->rom+(0x4000*(mbc->current_rombank)), 0x4000);
        }
        else if((0x3000 <= addr) && (addr < 0x4000)) { // ROMBank switch
            mbc->current_rombank &= 0x00FF;
            mbc->current_rombank |= (value & 0x1) << 8;
            mbc->current_rombank = mbc->current_rombank % n_rombanks(s);
            memcpy(s->ram+0x4000, s->rom+(0x4000*(mbc->current_rombank)), 0x4000);
        }
        else if((0x4000 <= addr) && (addr < 0x6000)) { 
            uint8_t bank_no = value & 0x0F;
            printf("Change RAMBANK! %x, %x, %x\n", n_rambanks(s), n_rombanks(s), value);
            memcpy(mbc->rambanks[mbc->current_rambank], &(s->ram[0xA000]), 0x2000);
            mbc->current_rambank = bank_no;
            memcpy(s->ram+0xA000, mbc->rambanks[mbc->current_rambank], 0x2000);
        }
    } else {
        if(!mbc->ram_enabled && addr > 0x8000){
            printf("accessing RAM when disabled!\n");
            return 0;
        }
        return s->ram[addr];
    }
}


void mbc_init(gb_t* s){
    s->mmu->mbc = calloc(1, sizeof(mbc_t));
    switch(s->rom[0x0147]){
        case 9:
            s->mmu->mbc->has_battery = 1;
        case NO_MBC:
        case 8:
            s->mmu->mbc_fn = &no_mbc;
            break;
        case MBC1+2:
            s->mmu->mbc->has_battery = 1;
        case MBC1:
        case MBC1+1:
            printf("MBC1\n");
            s->mmu->mbc_fn = &mbc1;
            break;
        case MBC5+2:
        case MBC5+5:
            s->mmu->mbc->has_battery = 1;
        case MBC5:
        case MBC5+1:
        case MBC5+3:
        case MBC5+4:
            printf("MBC5\n");
            s->mmu->mbc_fn = &mbc5;
            break;
        case MBC3_TIMER_RAM_BATTERY:
            printf("MBC3\n");
            s->mmu->mbc_fn = &mbc3;
            break;
        default:
            printf("Error: Unknown membank %x!\n", s->ram[0x0147]);
            exit(0);
    }
}