#ifndef INSTR_HELP_
#define INSTR_HELP_

#include <assert.h>
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

uint16_t set_reg_from_bits(uint8_t idx, uint8_t val, gb_t* s);

#endif // INSTR_HELP_
