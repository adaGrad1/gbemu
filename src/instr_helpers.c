#include <assert.h>
#include <stdio.h>

#include "jtest.h"
#include "util.h"
#include "instr_helpers.h"

const uint8_t BITS_TO_REG_IDX[] = {
    RB,
    RC,
    RD,
    RE,
    RH,
    RL,
    255,
    RA
};

// uint8_t load_mem_at_regs(gb_t* s, uint8_t hi_reg_idx) {
//     uint16_t HL = (s->reg[hi_reg_idx] << 8) + (s->reg[hi_reg_idx+1]);
//     return s->ram[HL];
// }

memgrb get_reg_from_bits(uint8_t bits, gb_t* s) {
    uint16_t cycles = 1;
    uint8_t reg_idx = BITS_TO_REG_IDX[bits];
    uint8_t val;
    uint16_t HL = (s->reg[RH] << 8) + (s->reg[RL]);
    if(reg_idx == 255){
        val = s->ram[HL];
        cycles = 2;
    } else{
        val = s->reg[reg_idx];
    }
    memgrb toret = {cycles, val};
    return toret;
}


// uint16_t load_reg_from_bits(uint8_t idx, uint8_t val, gb_t* s) {
//     return load_reg_from_bits(idx, val, s, RH);
// }
uint16_t set_reg_from_bits(uint8_t idx, uint8_t val, gb_t* s) {
    uint16_t cycles = 1;
    uint8_t reg_idx = BITS_TO_REG_IDX[idx];
    uint16_t HL = (s->reg[RH] << 8) + (s->reg[RL]);
    if(reg_idx == 255){
        s->ram[HL] = val;
        cycles = 2;

    } else{
        s->reg[reg_idx] = val;
    }
    return cycles;
}


void set_flags(gb_t* s, uint8_t zero, uint8_t sub, uint8_t halfcarry, uint8_t carry){
    if(zero != LEAVE_BIT_AS_IS) set_bit(s->reg[RF], ZERO_FLAG_BIT, zero);
    if(sub != LEAVE_BIT_AS_IS) set_bit(s->reg[RF], SUB_FLAG_BIT, sub);
    if(halfcarry != LEAVE_BIT_AS_IS) set_bit(s->reg[RF], HALFCARRY_FLAG_BIT, halfcarry);
    if(carry != LEAVE_BIT_AS_IS) set_bit(s->reg[RF], CARRY_FLAG_BIT, carry);
}