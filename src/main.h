#ifndef MAIN_H
#define MAIN_H
// #define VERBOSE 1
#define GBR_LEN 0x10000 /* 4096 */
#include <stdlib.h>

enum REG_IDX {
RA = 0,
RF,
RB,
RC,
RD,
RE,
RH,
RL,
REG_LEN,
};

typedef struct gbstate {
    uint16_t pc;
    uint16_t sp;
    uint8_t ei;
    uint64_t cycles;
    uint64_t total_cycles;
    unsigned char reg[REG_LEN];
    unsigned char ram[GBR_LEN];
} gb_t;

#endif