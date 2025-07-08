#include <assert.h>
#include <stdio.h>

#include "jtest.h"
#include "util.h"


// 0 1 [x1 x2 x3] [y1 y2 y3]
void ld(uint8_t instr, struct gbstate *s) {
    uint8_t bits_to_reg_idx[] = {
        RB,
        RC,
        RD,
        RE,
        RH,
        RL,
        255,
        RA
    };
    uint8_t source_reg_idx = bits_to_reg_idx[(instr & 0x07)];
    uint8_t source;
    uint16_t HL = (s->reg[RH] << 8) + (s->reg[RL]);
    if(source_reg_idx == 255){
        source = s->ram[HL];
    } else{
        source = s->reg[source_reg_idx];
    }

    uint8_t target_reg_idx = bits_to_reg_idx[((instr >> 3) & 0x7)];

    if(target_reg_idx == 255){
        s->ram[HL] = source;
    } else{
        s->reg[target_reg_idx] = source;
    }
}

void add(uint8_t instr, struct gbstate *s) {
    uint8_t source;
    switch (instr) {
        case 0x80: { source = s->reg[RB]; break; }
        case 0x81: { source = s->reg[RC]; break; }
        case 0x82: { source = s->reg[RD]; break; }
        case 0x83: { source = s->reg[RE]; break; }
        case 0x84: { source = s->reg[RH]; break; }
        case 0x85: { source = s->reg[RL]; break; }
        /*case 0x86: { assert("unsupported opcode"); break; }*/
        case 0x87: { source = s->reg[RA]; break; }
        default: break;
    }

    s->reg[RA] += source;
    s->pc++;
}

void sub(uint8_t instr, struct gbstate *s) {
    uint8_t source_reg = 0;
    switch (instr) {
        case 0x90: { source_reg = s->reg[RB]; } break;
        case 0x91: { source_reg = s->reg[RC]; } break;
        case 0x92: { source_reg = s->reg[RD]; } break;
        case 0x93: { source_reg = s->reg[RE]; } break;
        case 0x94: { source_reg = s->reg[RH]; } break;
        case 0x95: { source_reg = s->reg[RL]; } break;
        /* case 0x96: assert(0 && "Unsupported sub istr"); // needs HL */
        case 0x97: { source_reg = s->reg[RA]; } break;
        default:
            error ("Unsupported sub instruction %02X", instr);
            assert(0);
    };
    s->reg[RA] -= source_reg;

    s->pc += 1;
}


void halt() {}

#define mop(instr, match, mask) ((instr & mask) == match)
void step(struct gbstate *s) {
    uint8_t instr = s->ram[s->pc++];
    if mop(instr, 0x76, 0xFF) halt();
    else if mop(instr, 0x40, 0xC0) ld(instr, s);
    else if mop(instr, 0x80, 0xF8) add(instr, s); //no carry
}
