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
