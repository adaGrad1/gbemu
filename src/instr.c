#include <assert.h>
#include <stdio.h>

#include "jtest.h"
#include "util.h"
#include "instr_helpers.h"

#define max(a,b) a > b ? a : b
uint16_t halt() {return 1;} // TODO reset-related??

uint16_t incdec(uint8_t instr, gb_t *s) {
    uint8_t reg_idx = (instr >> 3) & 0x07;
    uint8_t delta = 1 - 2 * (instr & 0x01);
    uint8_t cycles = 0;
    memgrb prev_val = get_reg_from_bits(reg_idx, s);
    cycles += prev_val.cycles;
    uint8_t sum = prev_val.val+delta;
    cycles += set_reg_from_bits(reg_idx, sum, s);
    // this is so stupid, why is this what halfcarry expects >_>
    set_flags(s, !sum, delta>127, (prev_val.val & 0x0F) == 0x0F, LEAVE_BIT_AS_IS);
    return cycles-1;
}


uint16_t ld_ext(uint8_t instr, gb_t *s) {
    // return 0;
    // which pair of registers to load -- BC / DE / HL

    static const uint8_t first_reg_table[] = {2, 4, 6, 6};
    static const int16_t delta_table[] = {0, 0, 1, -1};

    uint8_t arg = (instr & 0x30) >> 4;
    uint8_t first_reg_idx = first_reg_table[arg];
    int16_t delta = delta_table[arg];

    // whether we are loading FROM or TO RAM.
    uint8_t from_mem = (instr & 0x08) >> 3;
    if(from_mem){
        s->reg[RA] = get_mem_at_reg(s, first_reg_idx);
    } else {
        set_mem_at_reg(s->reg[RA], s, first_reg_idx);
    }
    adi_regpair(delta, s, first_reg_idx);

    return 2; // TODO
}

uint16_t ldi(uint8_t instr, gb_t *s) {
    uint8_t reg_idx = (instr & 0x38) >> 3; // bits 
    uint8_t val = s->ram[s->pc++];
    return 1+set_reg_from_bits(reg_idx, val, s);
}


uint16_t ld(uint8_t instr, gb_t *s) {
    // 0 1 [x1 x2 x3] [y1 y2 y3]
    memgrb source = get_reg_from_bits((instr & 0x07), s);
    uint16_t set_cycles = set_reg_from_bits(((instr >> 3) & 0x7), source.val, s);
    return max(source.cycles, set_cycles);
}

uint16_t add(uint8_t instr, gb_t *s) {
    memgrb source = get_reg_from_bits((instr & 0x07), s);
    uint8_t carry_to_include = (instr & 0x08) && get_bit(s->reg[RF], CARRY_FLAG_BIT);
    uint8_t carry = ((uint16_t)s->reg[RA]) + ((uint16_t)source.val) + carry_to_include > 255;
    uint8_t halfcarry = (s->reg[RA] & 0x0F) + (source.val & 0x0F) + carry_to_include > 0x0F;
    s->reg[RA] += source.val + carry_to_include;
    set_flags(s, !s->reg[RA], 0, halfcarry, carry);
    return source.cycles;
}

uint16_t sub(uint8_t instr, gb_t *s) {
    memgrb source = get_reg_from_bits((instr & 0x07), s);
    uint8_t carry_to_include = (instr & 0x08) * get_bit(s->reg[RF], CARRY_FLAG_BIT);
    uint8_t carry = s->reg[RA] < (source.val + carry_to_include);
    uint8_t halfcarry = (s->reg[RA] & 0x0F) < (source.val & 0x0F) + carry_to_include;
    s->reg[RA] -= source.val + carry_to_include;
    set_flags(s, !s->reg[RA], 1, halfcarry, carry);
    return source.cycles;
}

uint16_t and(uint8_t instr, gb_t *s) {
    memgrb source = get_reg_from_bits((instr & 0x07), s);
    s->reg[RA] &= source.val;
    set_flags(s, !s->reg[RA], 0, 1, 0);
    return source.cycles;
}

uint16_t xor(uint8_t instr, gb_t *s) {
    memgrb source = get_reg_from_bits((instr & 0x07), s);
    s->reg[RA] ^= source.val;
    set_flags(s, !s->reg[RA], 0, 0, 0);
    return source.cycles;

}

uint16_t or(uint8_t instr, gb_t *s) {
    memgrb source = get_reg_from_bits((instr & 0x07), s);
    s->reg[RA] |= source.val;
    set_flags(s, !s->reg[RA], 0, 0, 0);
    return source.cycles;
}

uint16_t cp(uint8_t instr, gb_t *s) { // equivalent to a subtraction without updating RA, only RF
    uint8_t prev_a = s->reg[RA];
    uint8_t sub_instr = 0x80 | (instr & 0x07);
    uint16_t cycles = sub(sub_instr, s);
    s->reg[RA] = prev_a;
    return cycles;
}

#define mop(instr, match, mask) ((instr & mask) == match)
uint16_t step(gb_t *s) {
    uint8_t instr = s->ram[s->pc++];
    uint16_t r;
    if      mop(instr, 0x00, 0xEF) r=1; // nop or stop -- TODO reset-related??
    else if mop(instr, 0x76, 0xFF) r=halt();
    else if mop(instr, 0x02, 0xC7) r=ld_ext(instr, s);
    else if mop(instr, 0x04, 0xC7) r=incdec(instr, s);
    else if mop(instr, 0x05, 0xC7) r=incdec(instr, s);
    else if mop(instr, 0x06, 0xC7) r=ldi(instr, s);
    else if mop(instr, 0x40, 0xC0) r=ld(instr, s);
    else if mop(instr, 0x80, 0xF0) r=add(instr, s);
    else if mop(instr, 0x90, 0xF0) r=sub(instr, s);
    else if mop(instr, 0xA0, 0xF8) r=and(instr, s);
    else if mop(instr, 0xA8, 0xF8) r=xor(instr, s);
    else if mop(instr, 0xB0, 0xF8) r=or(instr, s);
    else if mop(instr, 0xB8, 0xF8) r=cp(instr, s);
    return r;
}
