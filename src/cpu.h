#ifndef CPU_H
#define CPU_H

#include "main.h"

void handle_interrupts(gb_t *s);
uint16_t step(gb_t *s);

#endif