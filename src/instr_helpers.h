#ifndef INSTR_HELP_
#define INSTR_HELP_
#define set_bit(x, b, v) x = (x & ~(1 << b)) | (!!v << b)
#define get_bit(x, b) (x >> b) & 1
#define ZERO_FLAG_BIT 7
#define SUB_FLAG_BIT 6
#define HALFCARRY_FLAG_BIT 5
#define CARRY_FLAG_BIT 4
#define LEAVE_BIT_AS_IS 255

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "jtest.h"
#include "util.h"
#include "instr.h"
#include "main.h"


typedef struct memory_grab {
    uint16_t cycles;
    uint8_t val;
} memgrb;

memgrb get_reg_from_bits(uint8_t bits, gb_t* s);
// memgrb set_reg_from_bits(uint8_t bits, gb_t* s, uint8_t reg_idx);

uint16_t set_reg_from_bits(uint8_t idx, uint8_t val, gb_t* s);
// uint16_t load_reg_from_bits(uint8_t idx, uint8_t val, gb_t* s, uint8_t reg_idx);

void set_flags(gb_t* s, uint8_t zero, uint8_t sub, uint8_t halfcarry, uint8_t carry);

#endif // INSTR_HELP_