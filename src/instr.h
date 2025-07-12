#ifndef INSTR_H_
#define INSTR_H_

#include <assert.h>
#include <stdio.h>

#include "jtest.h"
#include "util.h"
#include "main.h"
// Main instruction execution
uint16_t stept(gb_t *s);

// Instruction handlers
uint16_t halt(uint8_t instr, gb_t *s);
uint16_t incdec_16(uint8_t instr, gb_t *s);
uint16_t incdec(uint8_t instr, gb_t *s);
uint16_t ld_ext(uint8_t instr, gb_t *s);
uint16_t ldi_16(uint8_t instr, gb_t *s);
uint16_t ldi(uint8_t instr, gb_t *s);
uint16_t ld(uint8_t instr, gb_t *s);
uint16_t add(uint8_t instr, gb_t *s);
uint16_t sub(uint8_t instr, gb_t *s);
uint16_t and(uint8_t instr, gb_t *s);
uint16_t xor(uint8_t instr, gb_t *s);
uint16_t or(uint8_t instr, gb_t *s);
uint16_t cp(uint8_t instr, gb_t *s);
uint16_t cb(uint8_t instr, gb_t *s);
uint16_t ra(uint8_t instr, gb_t *s);
uint16_t arith_i(uint8_t instr, gb_t *s);
uint16_t add_16(uint8_t instr, gb_t *s);
uint16_t scf(uint8_t instr, gb_t *s);
uint16_t ccf(uint8_t instr, gb_t *s);
uint16_t cpl(uint8_t instr, gb_t *s);
uint16_t daa(uint8_t instr, gb_t *s);
uint16_t jmp(uint8_t instr, gb_t *s);
uint16_t di(uint8_t instr, gb_t *s);
uint16_t ei(uint8_t instr, gb_t *s);
uint16_t ld_hl_sp(uint8_t instr, gb_t *s);
uint16_t ld_sp_hl(uint8_t instr, gb_t *s);
uint16_t ld_ia(uint8_t instr, gb_t *s);
uint16_t ld_rc_a(uint8_t instr, gb_t *s);
uint16_t adi_sp(uint8_t instr, gb_t *s);
uint16_t jp_hl(uint8_t instr, gb_t *s);
uint16_t callcond(uint8_t instr, gb_t *s);
uint16_t call(uint8_t instr, gb_t *s);
uint16_t jr(uint8_t instr, gb_t *s);
uint16_t stop(uint8_t instr, gb_t *s);
uint16_t ld_ia_sp(uint8_t instr, gb_t *s);
uint16_t pop(uint8_t instr, gb_t *s);
uint16_t push(uint8_t instr, gb_t *s);
uint16_t retcond(uint8_t instr, gb_t *s);
uint16_t ret(uint8_t instr, gb_t *s);
uint16_t reti(uint8_t instr, gb_t *s);
uint16_t rst(uint8_t instr, gb_t *s);

// CB prefix instruction handlers
void cb_bit(uint8_t ri, uint8_t instr, gb_t *s);
void cb_res(uint8_t ri, uint8_t instr, gb_t *s);
void cb_set(uint8_t ri, uint8_t instr, gb_t *s);
void cb_shift_rotate(uint8_t ri, gb_t *s, uint8_t left, uint8_t from_carry, uint8_t rotate, uint8_t signed_shr);
void rlc(uint8_t ri, uint8_t instr, gb_t *s);
void rrc(uint8_t ri, uint8_t instr, gb_t *s);
void rl(uint8_t ri, uint8_t instr, gb_t *s);
void rr(uint8_t ri, uint8_t instr, gb_t *s);
void sl(uint8_t ri, uint8_t instr, gb_t *s);
void sr(uint8_t ri, uint8_t instr, gb_t *s);
void swap(uint8_t ri, uint8_t instr, gb_t *s);
void srl(uint8_t ri, uint8_t instr, gb_t *s);

// Helper functions
void _ret(gb_t *s);
void _call(uint16_t addr, gb_t *s);
#endif // INSTR_H_
