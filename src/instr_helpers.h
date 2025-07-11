#ifndef INSTR_HELP_
#define INSTR_HELP_
#define set_bit(x, b, v) x = ((x) & ~(1 << (b))) | (!!(v) << (b))
#define get_bit(x, b) (((x) >> (b)) & 1)
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

void adi_regpair(int16_t val, gb_t* s, uint8_t reg_idx); // add immediate to a particular pair of registers (e.g. HL or AF);
memgrb get_reg_from_bits(uint8_t bits, gb_t* s);
uint8_t get_mem_at_reg(gb_t* s, uint8_t reg_idx);

uint16_t set_reg_from_bits(uint8_t idx, uint8_t val, gb_t* s);
void set_mem_at_reg(uint8_t val, gb_t* s, uint8_t reg_idx);

uint16_t load_reg_from_bits(uint8_t idx, uint8_t val, gb_t* s, uint8_t reg_idx);

void set_flags(gb_t* s, uint8_t zero, uint8_t sub, uint8_t halfcarry, uint8_t carry);

uint8_t calc_add_carry_8(uint8_t a, uint8_t b, uint8_t carry_in);
uint8_t calc_add_halfcarry_8(uint8_t a, uint8_t b, uint8_t carry_in);
uint8_t calc_sub_carry_8(uint8_t a, uint8_t b, uint8_t carry_in);
uint8_t calc_sub_halfcarry_8(uint8_t a, uint8_t b, uint8_t carry_in);
uint8_t calc_add_carry_16(uint16_t a, uint16_t b);
uint8_t calc_add_halfcarry_16(uint16_t a, uint16_t b);

#endif // INSTR_HELP_