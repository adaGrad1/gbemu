#include <raylib.h>
#include "mmu.h"
#include "mbc.h"
#include "instr_helpers.h"

void mmu_init(gb_t* s, char* rom_path){
    s->sp = 0xFFFE;
    memcpy(s->ram, s->rom, 0x8000); 
    s->mmu = calloc(1, sizeof(mmu_t));
    mbc_init(s);
    char* savepath = calloc(1, sizeof(rom_path)+10);
    strcpy(savepath, rom_path);
    strcat(savepath, ".save");
    FILE* f = fopen(savepath, "rb");
    if (f){
        fread(s->mmu->mbc->rambanks, sizeof(s->mmu->mbc->rambanks), 1, f);
        fclose(f);
    }
    printf("RAMBANK LOADED AMOUNT: %x. At 0x0454: %x\n", sizeof(s->mmu->mbc->rambanks), s->mmu->mbc->rambanks[0][0x454]);
}

void save_persistent(gb_t* s, char* rom_path){
    // return;
    if (s->mmu->mbc->has_battery){
        printf("SAVE PERSISTENT\n");
        char* savepath = calloc(1, sizeof(rom_path)+10);
        strcpy(savepath, rom_path);
        strcat(savepath, ".save");
        printf("%s\n", savepath);
        FILE* f = fopen(savepath, "wb");
        fwrite(s->mmu->mbc->rambanks, 1, sizeof(s->mmu->mbc->rambanks), f);
        fclose(f);
        free(savepath);
    }
}

const uint16_t freqs[] = {256, 4, 16, 64};
uint16_t oam_dma(gb_t *s){
  uint16_t bytes_to_copy = 0x100;
  uint16_t start_addr = (s->ram[0xFF46]) << 8;
  uint16_t OAM_start = 0xFE00;

  for(uint16_t i = 0; i < bytes_to_copy; i++){
    s->ram[OAM_start+i] = s->ram[start_addr+i];
  }
  s->ram[0xFF46] = 0;
}

uint8_t get_ticks(uint64_t prev_cycles, uint64_t cycles_passed, uint16_t timer_wavelen){
    return ((prev_cycles-cycles_passed)%timer_wavelen) - ((prev_cycles)%timer_wavelen);
}
void update_timer(gb_t* s, uint16_t cycles_passed){
    s->ram[0xFF04] += get_ticks(s->cycles_total, cycles_passed, 256); // DIV
    uint8_t update_tima = get_bit(get_mem(s, 0xFF07), 2);
    if (update_tima){
        uint16_t timer_wavelen = freqs[get_mem(s, 0xFF07) & 0x03];
        uint16_t tima_ticks = get_ticks(s->cycles_total, cycles_passed, timer_wavelen);
        if (tima_ticks + get_mem(s, 0xFF05) > 0xFF){
            s->ram[0xFF05] = s->ram[0xFF06] + tima_ticks + s->ram[0xFF05]; // set to modulo
            set_bit(s->ram[0xFF0F], 2, 1); // request timer interrupt
        }
        
    }
}

void update_joypad(gb_t* s){
    uint8_t joypad_flags = (s->ram[0xFF00] >> 4) & 0x3;
    uint8_t prev_joypad = s->ram[0xFF00];
    switch(joypad_flags){
        case 0:
        case 3:
            s->ram[0xFF00] |= 0x0F;
            break;
        case 2: // dpad
            set_bit(s->ram[0xFF00], 0, !IsKeyDown(KEY_RIGHT));
            set_bit(s->ram[0xFF00], 1, !IsKeyDown(KEY_LEFT));
            set_bit(s->ram[0xFF00], 2, !IsKeyDown(KEY_UP));
            set_bit(s->ram[0xFF00], 3, !IsKeyDown(KEY_DOWN));
            break;
        case 1:
            set_bit(s->ram[0xFF00], 0, !IsKeyDown(KEY_Z));
            set_bit(s->ram[0xFF00], 1, !IsKeyDown(KEY_X));
            set_bit(s->ram[0xFF00], 2, !IsKeyDown(KEY_RIGHT_SHIFT));
            set_bit(s->ram[0xFF00], 3, !IsKeyDown(KEY_ENTER));
    }
    if (s->ram[0xFF00] != prev_joypad) set_bit(s->ram[0xFF0F], 4, 1);
}

uint8_t get_mem(gb_t* s, uint16_t addr) {
    if (s->test_mode) return s->ram[addr];
    if(addr < 0xC000){
        return (*(s->mmu->mbc_fn))(s, addr, 0, 0);
    }
    if((0xE000 <= addr) && (addr < 0xFE00)) { // Echo RAM
        return s->ram[addr-0x2000];
    }
    else if((addr >= 0xFF00) && (addr < 0xFF80)) { // MMIO
        // TODO serial transfer
        // TODO timer / divider
        // TODO audio
        // TODO wave pattern
        // TODO LCD flags
        // TODO probably more https://gbdev.io/pandocs/Memory_Map.html#io-ranges
        if (addr == 0xFF00) {
            update_joypad(s);
        } else if (addr == 0xFF46){
            oam_dma(s);
        }
    }
    return s->ram[addr];
}



void set_mem(gb_t* s, uint16_t addr, uint8_t value) {
    if (s->test_mode){
        s->ram[addr] = value;
        return;
    }
    if(addr < 0x8000){ // ROM -- can't set
        // uint8_t bank_no = value & 0x1F;
        // if(bank_no == 0) bank_no = 1;
        // memcpy(s->ram+0x4000, s->rom+(0x4000*bank_no), 0x4000); 
        (*(s->mmu->mbc_fn))(s, addr, value, 1);
        return;
    }
    else if((0xE000 <= addr) && (addr < 0xFE00)) { // Echo RAM
        s->ram[addr-0x2000] = value;
    }
    else if((0xFEA0 <= addr) && (addr < 0xFEFF)) {
        return; // Not usable!
    }
    else if((addr >= 0xFF00) && (addr < 0xFF80)) { // MMIO
        // TODO serial transfer
        // TODO timer / divider
        // TODO audio
        // TODO wave pattern
        // TODO LCD flags
        // TODO probably more https://gbdev.io/pandocs/Memory_Map.html#io-ranges
        if (addr == 0xFF04){
            s->ram[0xFF04] = 0;
        } else if (addr == 0xFF46){
            s->ram[addr] = value;
            oam_dma(s);
        } else {
            s->ram[addr] = value;
        }
    } else { // regular RAM/VRAM/OAM/HighRAM
        s->ram[addr] = value;
    }
}