#include <assert.h>
#include <stdio.h>

#include "jtest.h"
#include "util.h"
#include "instr_helpers.h"

#define max(a,b) a > b ? a : b

uint16_t ld(uint8_t instr, gb_t *s) {
    // 0 1 [x1 x2 x3] [y1 y2 y3]
    memgrb source = get_reg_from_bits((instr & 0x07), s);
    uint16_t set_cycles = set_reg_from_bits(((instr >> 3) & 0x7), source.val, s);
    return max(source.cycles, set_cycles);
}

uint16_t add(uint8_t instr, gb_t *s) {
    memgrb source = get_reg_from_bits((instr & 0x07), s);

    s->reg[RA] += source.val;
}

// uint16_t sub(uint8_t instr, gb_t *s) {
//     uint8_t source_reg_idx = 
//     switch (instr) {
//         case 0x90: { source_reg = s->reg[RB]; } break;
//         case 0x91: { source_reg = s->reg[RC]; } break;
//         case 0x92: { source_reg = s->reg[RD]; } break;
//         case 0x93: { source_reg = s->reg[RE]; } break;
//         case 0x94: { source_reg = s->reg[RH]; } break;
//         case 0x95: { source_reg = s->reg[RL]; } break;
//         /* case 0x96: assert(0 && "Unsupported sub istr"); // needs HL */
//         case 0x97: { source_reg = s->reg[RA]; } break;
//         default:
//             error ("Unsupported sub instruction %02X", instr);
//             assert(0);
//     };
//     s->reg[RA] -= source_reg;

//     s->pc += 1;
// }


uint16_t halt() {return 0;}

#define mop(instr, match, mask) ((instr & mask) == match)
uint16_t step(gb_t *s) {
    uint8_t instr = s->ram[s->pc++];
    uint16_t r;
    if mop(instr, 0x76, 0xFF) r=halt();
    else if mop(instr, 0x40, 0xC0) r=ld(instr, s);
    else if mop(instr, 0x80, 0xF8) r=add(instr, s); //no carry
    return r;
}
