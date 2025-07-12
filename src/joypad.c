#include <stdio.h>
#include <raylib.h>

#include "util.h"
#include "main.h"
#include "joypad.h"
#include "instr_helpers.h"

void update_joypad(gb_t* s){
    uint8_t joypad_flags = (s->ram[0xFF00] >> 4) & 0x3;
    switch(joypad_flags){
        case 0:
        case 3:
            s->ram[0xFF00] |= 0x0F;
            break;
        case 1: // dpad
            set_bit(s->ram[0xFF00], 0, !IsKeyDown(KEY_RIGHT));
            set_bit(s->ram[0xFF00], 1, !IsKeyDown(KEY_LEFT));
            set_bit(s->ram[0xFF00], 2, !IsKeyDown(KEY_UP));
            set_bit(s->ram[0xFF00], 3, !IsKeyDown(KEY_DOWN));
            break;
        case 2:
            set_bit(s->ram[0xFF00], 0, !IsKeyDown(KEY_Z));
            set_bit(s->ram[0xFF00], 1, !IsKeyDown(KEY_X));
            set_bit(s->ram[0xFF00], 2, !IsKeyDown(KEY_RIGHT_SHIFT));
            set_bit(s->ram[0xFF00], 3, !IsKeyDown(KEY_ENTER));
            printf("%x\n", s->ram[0xFF00]);
    }
}