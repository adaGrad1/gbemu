#ifndef INSTR_H_
#define INSTR_H_

#include <assert.h>
#include <stdio.h>

#include "jtest.h"
#include "util.h"
#include "main.h"

// Returns the number of CPU cycles taken.
uint16_t step(gb_t *s);
#endif // INSTR_H_
