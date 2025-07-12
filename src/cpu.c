#include "main.h"
#include "instr.h"
#include "instr_helpers.h"

#define mop(instr, match, mask) ((instr & mask) == match)

void handle_interrupts(gb_t *s){
  if (s->ei && (s->ram[0xFF0F] & s->ram[0xFFFF])) {
      s->ram[--(s->sp)] = s->pc >> 8;
      s->ram[--(s->sp)] = s->pc & 0xFF;

      if (s->ram[0xFF0F] & 1) {
        s->pc = 0x0040;
        set_bit(s->ram[0xFF0F], 0, 0);
      }
      s->ei = 0;
  }
}

uint16_t oam_dma(gb_t *s){
  // printf("DMA OAM\n");
  uint16_t bytes_to_copy = 0x100;
  uint16_t start_addr = (s->ram[0xFF46]) << 8;
  uint16_t OAM_start = 0xFE00;

  for(uint16_t i = 0; i < bytes_to_copy; i++){
    s->ram[OAM_start+i] = s->ram[start_addr+i];
  }

  s->ram[0xFF46] = 0;
}

uint16_t step(gb_t *s) {
    uint8_t instr = s->ram[s->pc++];
    uint16_t r;
    if      mop(instr, 0x00, 0xFF) r=1;
    else if mop(instr, 0xFB, 0xFF) r=ei(instr, s);
    else if mop(instr, 0x08, 0xFF) r=ld_ia_sp(instr, s); // TODO
    else if mop(instr, 0x10, 0xFF) r=stop(instr, s);
    else if mop(instr, 0x07, 0xE7) r=ra(instr, s);
    else if mop(instr, 0x01, 0xCF) r=ldi_16(instr, s);
    else if mop(instr, 0x08, 0xCF) r=jr(instr, s);
    else if mop(instr, 0x20, 0xEF) r=jr(instr, s);
    else if mop(instr, 0x27, 0xFF) r=daa(instr, s);
    else if mop(instr, 0x37, 0xFF) r=scf(instr, s);
    else if mop(instr, 0x2F, 0xFF) r=cpl(instr, s);
    else if mop(instr, 0x3F, 0xFF) r=ccf(instr, s);
    else if mop(instr, 0x76, 0xFF) r=halt(instr, s);
    else if mop(instr, 0xF8, 0xFF) r=ld_hl_sp(instr,s);
    else if mop(instr, 0xF9, 0xFF) r=ld_sp_hl(instr,s);
    else if mop(instr, 0xF3, 0xFF) r=di(instr, s);
    else if mop(instr, 0x02, 0xC7) r=ld_ext(instr, s);
    else if mop(instr, 0x03, 0xC7) r=incdec_16(instr, s);
    else if mop(instr, 0x04, 0xC7) r=incdec(instr, s);
    else if mop(instr, 0x05, 0xC7) r=incdec(instr, s);
    else if mop(instr, 0x06, 0xC7) r=ldi(instr, s);
    else if mop(instr, 0x09, 0xCF) r=add_16(instr, s);
    else if mop(instr, 0x40, 0xC0) r=ld(instr, s);
    else if mop(instr, 0x80, 0xF0) r=add(instr, s);
    else if mop(instr, 0x90, 0xF0) r=sub(instr, s);
    else if mop(instr, 0xA0, 0xF8) r=and(instr, s);
    else if mop(instr, 0xA8, 0xF8) r=xor(instr, s);
    else if mop(instr, 0xC6, 0xC7) r=arith_i(instr, s);
    else if mop(instr, 0xB0, 0xF8) r=or(instr, s);
    else if mop(instr, 0xB8, 0xF8) r=cp(instr, s);
    else if mop(instr, 0xCB, 0xFF) r=cb(instr, s);
    else if mop(instr, 0xC1, 0xCF) r=pop(instr, s);
    else if mop(instr, 0xC3, 0xFF) r=jmp(instr, s);
    else if mop(instr, 0xC5, 0xCF) r=push(instr, s);
    else if mop(instr, 0xC0, 0xE7) r=retcond(instr, s);
    else if mop(instr, 0xC9, 0xFF) r=ret(instr, s);
    else if mop(instr, 0xD9, 0xFF) r=reti(instr, s);
    else if mop(instr, 0xC4, 0xE7) r=callcond(instr, s);
    else if mop(instr, 0xCD, 0xFF) r=call(instr, s);
    else if mop(instr, 0xC2, 0xE7) r=jmp(instr, s);
    else if mop(instr, 0xC7, 0xC7) r=rst(instr, s);
    else if mop(instr, 0xE0, 0xEF) r=ld_ia(instr, s);
    else if mop(instr, 0xEA, 0xEF) r=ld_ia(instr, s);
    else if mop(instr, 0xE2, 0xEF) r=ld_rc_a(instr, s);
    else if mop(instr, 0xE8, 0xFF) r=adi_sp(instr, s);
    else if mop(instr, 0xE9, 0xFF) r=jp_hl(instr, s);
    else printf("unknown opcode!!\n");

    if (s->ram[0xFF46] && !s->test_mode){
      oam_dma(s);
    }

    return r;
}