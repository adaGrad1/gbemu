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

uint8_t get_mem_at_reg(gb_t* s, uint8_t reg_idx){
    uint16_t ptr = s->reg[reg_idx];
    ptr <<= 8;
    ptr += s->reg[reg_idx+1];
    return s->ram[ptr];
}

memgrb get_reg_from_bits(uint8_t bits, gb_t* s) {
    uint16_t cycles = 1;
    uint8_t reg_idx = BITS_TO_REG_IDX[bits];
    uint8_t val;
    if(reg_idx == 255){
        val = get_mem_at_reg(s, RH);
        cycles = 2;
    } else{
        val = s->reg[reg_idx];
    }
    memgrb toret = {cycles, val};
    return toret;
}

// add immediate to a pair of registers
void adi_regpair(int16_t val, gb_t* s, uint8_t reg_idx){
    // it would be most convenient to do uint16_t* val = (uint16_t) ((s->reg)+reg_idx);
    // but I think this breaks depending on the endian-ness of the system
    // so let's just not!
    uint16_t old_val = (s->reg[reg_idx] << 8) + s->reg[reg_idx+1];
    uint16_t new_val = old_val + val;
    s->reg[reg_idx] = new_val >> 8;
    s->reg[reg_idx+1] = new_val & 0x00FF;
}
void set_mem_at_reg(uint8_t val, gb_t* s, uint8_t reg_idx) {
    uint16_t ptr = (s->reg[reg_idx] << 8) + (s->reg[reg_idx+1]);
    s->ram[ptr] = val;
}

uint16_t set_reg_from_bits(uint8_t idx, uint8_t val, gb_t* s) {
    uint16_t cycles = 1;
    uint8_t reg_idx = BITS_TO_REG_IDX[idx];
    if(reg_idx == 255){
        set_mem_at_reg(val, s, RH);
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