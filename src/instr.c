#include <assert.h>
#include <stdio.h>

#include "util.h"
#include "instr_helpers.h"
#include "cpu.h"
#include "mmu.h"
#define mop(instr, match, mask) ((instr & mask) == match)

#define max(a,b) a > b ? a : b
uint16_t halt(uint8_t instr, gb_t *s) {s->halt_mode=1; return 3;}

uint16_t incdec_16(uint8_t instr, gb_t *s) {
    uint8_t reg_idx = 2 * ((instr >> 4) & 0x03);
    int16_t delta = 1 - 2 * !!(instr & 0x8);
    uint8_t SP_case = (((instr >> 4) & 0x03) == 0x03);
    if (SP_case) {
        s->sp += delta;
    } else {
        uint16_t val = get_reg_from_bits(reg_idx, s).val;
        val <<= 8;
        val += get_reg_from_bits(reg_idx+1, s).val;
        val += delta;
        set_reg_from_bits(reg_idx, val >> 8, s);
        set_reg_from_bits(reg_idx+1, val & 0x00FF, s);
    }
    return 2;
}

uint16_t incdec(uint8_t instr, gb_t *s) {
    uint8_t reg_idx = (instr >> 3) & 0x07;
    int8_t delta = 1 - 2 * (instr & 0x01);
    uint8_t cycles = 0;
    memgrb prev_val = get_reg_from_bits(reg_idx, s);
    cycles += prev_val.cycles;
    uint8_t sum = prev_val.val+delta;
    cycles += set_reg_from_bits(reg_idx, sum, s);
    uint8_t hc;
    if (delta > 0){
        hc = calc_add_halfcarry_8(prev_val.val, delta, 0);
    } else {
        hc = calc_sub_halfcarry_8(prev_val.val, -delta, 0);
    }
    set_flags(s, !sum, delta<0, hc, LEAVE_BIT_AS_IS);
    return cycles-1;
}
uint16_t ld_ext(uint8_t instr, gb_t *s) {
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

uint16_t ldi_16(uint8_t instr, gb_t *s) {
    uint16_t val = get_mem(s, s->pc+1);
    val <<= 8;
    val += get_mem(s, s->pc);
    s->pc+=2;

    uint8_t reg_idx = 2 * ((instr >> 4) & 0x03);
    uint8_t SP_case = (((instr >> 4) & 0x03) == 0x03);
    if (SP_case) {
        s->sp = val;
    } else {
        set_reg_from_bits(reg_idx, val >> 8, s);
        set_reg_from_bits(reg_idx+1, val & 0x00FF, s);
    }

    return 3;
}


uint16_t ldi(uint8_t instr, gb_t *s) {
    uint8_t reg_idx = (instr >> 3) & 0x07; // bits 
    uint8_t val = get_mem(s, s->pc++);
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
    uint8_t carry = calc_add_carry_8(s->reg[RA], source.val, carry_to_include);
    uint8_t halfcarry = calc_add_halfcarry_8(s->reg[RA], source.val, carry_to_include);
    s->reg[RA] += source.val + carry_to_include;
    set_flags(s, !s->reg[RA], 0, halfcarry, carry);
    return source.cycles;
}

uint16_t sub(uint8_t instr, gb_t *s) {
    memgrb source = get_reg_from_bits((instr & 0x07), s);
    uint8_t carry_to_include = (instr & 0x08) && get_bit(s->reg[RF], CARRY_FLAG_BIT);
    uint8_t carry = calc_sub_carry_8(s->reg[RA], source.val, carry_to_include);
    uint8_t halfcarry = calc_sub_halfcarry_8(s->reg[RA], source.val, carry_to_include);
    s->reg[RA] -= (source.val + carry_to_include);
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

void cb_bit(uint8_t ri, uint8_t instr, gb_t *s){
    uint8_t bit_idx = (instr >> 3) & 0x07;
    uint8_t reg_val = get_reg_from_bits(ri, s).val;
    set_flags(s, !get_bit(reg_val, bit_idx), 0, 1, LEAVE_BIT_AS_IS);
}


void cb_res(uint8_t ri, uint8_t instr, gb_t *s){
    uint8_t bit_idx = (instr >> 3) & 0x07;
    uint8_t reg_val = get_reg_from_bits(ri, s).val;
    reg_val &= ~(1 << bit_idx);
    set_reg_from_bits(ri, reg_val, s);
}
void cb_set(uint8_t ri, uint8_t instr, gb_t *s){
    uint8_t bit_idx = (instr >> 3) & 0x07;
    uint8_t reg_val = get_reg_from_bits(ri, s).val;
    reg_val |= (1 << bit_idx);
    set_reg_from_bits(ri, reg_val, s);
}

void cb_shift_rotate(uint8_t ri, gb_t* s, uint8_t left, uint8_t from_carry, uint8_t rotate, uint8_t signed_shr){
    uint8_t v = get_reg_from_bits(ri, s).val;
    uint8_t new_carry;
    if (left) {
        new_carry = get_bit(v, 7);
        v <<= 1;
        if (rotate) {
            set_bit(v, 0, new_carry);
        } else if (from_carry) {
            set_bit(v, 0, get_bit(s->reg[RF], CARRY_FLAG_BIT));
        }
    }
    else {
        new_carry = get_bit(v, 0);
        v >>= 1;
        if (rotate) {
            set_bit(v, 7, new_carry);
        } else if (from_carry) {
            set_bit(v, 7, get_bit(s->reg[RF], CARRY_FLAG_BIT));
        } else if (signed_shr) {
                set_bit(v, 7, (get_bit(v, 6)));
        }
    }
    set_reg_from_bits(ri, v, s);
    set_flags(s, !v, 0, 0, new_carry);
}

void rlc(uint8_t ri, uint8_t instr, gb_t *s){
    cb_shift_rotate(ri, s, 1, 0, 1, 0);
}

void rrc(uint8_t ri, uint8_t instr, gb_t *s){
    cb_shift_rotate(ri, s, 0, 0, 1, 0);
}

void rl(uint8_t ri, uint8_t instr, gb_t *s){
    cb_shift_rotate(ri, s, 1, 1, 0, 0);
}

void rr(uint8_t ri, uint8_t instr, gb_t *s){
    cb_shift_rotate(ri, s, 0, 1, 0, 0);
}

void sl(uint8_t ri, uint8_t instr, gb_t *s){
    cb_shift_rotate(ri, s, 1, 0, 0, 0);
}

void sr(uint8_t ri, uint8_t instr, gb_t *s){
    cb_shift_rotate(ri, s, 0, 0, 0, 1);
}

void swap(uint8_t ri, uint8_t instr, gb_t *s){
    uint8_t new = 0;
    uint8_t old = get_reg_from_bits(ri, s).val;
    new += old >> 4;
    new += old << 4;
    set_reg_from_bits(ri, new, s);
    set_flags(s, !new, 0, 0, 0);
}

void srl(uint8_t ri, uint8_t instr, gb_t *s){
    cb_shift_rotate(ri, s, 0, 0, 0, 0);
}

uint16_t pop(uint8_t instr, gb_t *s) 
{   
    uint8_t reg_idx = 2 * ((instr >> 4) & 0x03);
    uint8_t AF_case = (((instr >> 4) & 0x03) == 0x03);
    if(AF_case) {
        s->reg[RF] = get_mem(s, s->sp++) & 0xF0;
        s->reg[RA] = get_mem(s, s->sp++);
    } else {
        set_reg_from_bits(reg_idx+1, get_mem(s, s->sp++), s);
        set_reg_from_bits(reg_idx, get_mem(s, s->sp++), s);
    }
    return 3;
}

uint16_t push(uint8_t instr, gb_t *s) 
{   
    uint8_t reg_idx = 2 * ((instr >> 4) & 0x03);
    uint8_t AF_case = (((instr >> 4) & 0x03) == 0x03);
    if(AF_case) {
        reg_idx += 1;
    }
        set_mem(s, --(s->sp), get_reg_from_bits(reg_idx, s).val);
        set_mem(s, --(s->sp), get_reg_from_bits(reg_idx+1, s).val);
    return 4;
}

void _ret(gb_t* s){
    s->pc = get_mem(s, (s->sp)+1);
    s->pc <<= 8;
    s->pc += get_mem(s, (s->sp));
    s->sp+=2;
}

uint16_t retcond(uint8_t instr, gb_t* s) {
    uint8_t bit_to_check = ((instr >> 4) & 0x01) ? CARRY_FLAG_BIT : ZERO_FLAG_BIT;

    uint8_t cond = !get_bit(s->reg[RF], bit_to_check);
    if ((instr >> 3) & 0x01){
        cond = !cond;
    }
    if (cond){
        _ret(s);
        return 5;
    } else {
        return 2;
    }
}

uint16_t ret(uint8_t instr, gb_t* s) {
    _ret(s);
    return 4;
}

uint16_t reti(uint8_t instr, gb_t* s) {
    _ret(s);
    s->ime = 1;
    return 4;
}


void _call(uint16_t addr, gb_t* s){
    set_mem(s, --(s->sp), s->pc >> 8);
    set_mem(s, --(s->sp), s->pc);
    s->pc = addr;
}

// RST is 1 1 [o1 o2 o3] 1 1 1
uint16_t rst(uint8_t instr, gb_t *s) {
    _call(instr & 0x38, s);
    return 4;
}

uint16_t cb(uint8_t _, gb_t *s) {
    uint8_t instr = get_mem(s, s->pc++);
    uint16_t cycles = 2;
    if ((instr & 0x07) == 0x06) { // instr involves RAM
        if((instr & 0xC0) == 0x40) cycles = 3; // only READ from RAM
        else cycles = 4; // READ and WRITE to RAM
    }
    uint8_t ri = instr & 0x07;
    if      mop(instr, 0x40, 0xC0) cb_bit(ri, instr, s);
    else if mop(instr, 0x80, 0xC0) cb_res(ri, instr, s);
    else if mop(instr, 0xC0, 0xC0) cb_set(ri, instr, s);
    // instr is 0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38
    else if mop(instr, 0x00, 0x38) rlc(ri, instr, s);
    else if mop(instr, 0x08, 0x38) rrc(ri, instr, s);
    else if mop(instr, 0x10, 0x38) rl(ri, instr, s);
    else if mop(instr, 0x18, 0x38) rr(ri, instr, s);
    else if mop(instr, 0x20, 0x38) sl(ri, instr, s);
    else if mop(instr, 0x28, 0x38) sr(ri, instr, s);
    else if mop(instr, 0x30, 0x38) swap(ri, instr, s);
    else if mop(instr, 0x38, 0x38) srl(ri, instr, s);
    return cycles;
}

uint16_t ra(uint8_t instr, gb_t* s) {
    s->pc--;
    cb(instr, s);
    set_flags(s, 0, LEAVE_BIT_AS_IS, LEAVE_BIT_AS_IS, LEAVE_BIT_AS_IS);
    return 1;
}


uint16_t arith_i(uint8_t instr, gb_t* s) {
    uint8_t b_tmp = s->reg[RB];
    uint8_t ins_tmp = get_mem(s, s->pc);
    s->reg[RB] = get_mem(s, s->pc);
    s->ram[s->pc] = (instr & 0xB8); // HACK!
    step(s);
    s->reg[RB] = b_tmp;
    s->ram[s->pc-1] = ins_tmp;
    return 2;
}

uint16_t add_16(uint8_t instr, gb_t *s) {
    uint8_t reg_idx = 2 * ((instr >> 4) & 0x03);
    uint8_t SP_case = (((instr >> 4) & 0x03) == 0x03);
    uint16_t src;
    if (SP_case) {
        src = s->sp;
    } else {
        src = get_reg_from_bits(reg_idx, s).val;
        src <<= 8;
        src += get_reg_from_bits(reg_idx+1, s).val;
    }
    uint16_t tgt = s->reg[RH];
    tgt <<= 8;
    tgt += s->reg[RL];
    uint8_t halfcarry = calc_add_halfcarry_16(tgt, src);
    uint8_t carry = calc_add_carry_16(tgt, src);    
    set_flags(s, LEAVE_BIT_AS_IS, 0, halfcarry, carry);

    tgt = tgt + src;
    s->reg[RH] = tgt >> 8;
    s->reg[RL] = tgt & 0xFF;
    return 2;
}

uint16_t scf(uint8_t instr, gb_t *s){
    set_flags(s, LEAVE_BIT_AS_IS, 0, 0, 1);
    return 1;
}

uint16_t ccf(uint8_t instr, gb_t *s){
    set_flags(s, LEAVE_BIT_AS_IS, 0, 0, !get_bit(s->reg[RF], CARRY_FLAG_BIT));
    return 1;
}

uint16_t cpl(uint8_t instr, gb_t *s){
    set_flags(s, LEAVE_BIT_AS_IS, 1, 1, LEAVE_BIT_AS_IS);
    s->reg[RA] = ~(s->reg[RA]);
    return 1;
}

uint16_t daa(uint8_t instr, gb_t *s){
    uint8_t sub_flag = get_bit(s->reg[RF], SUB_FLAG_BIT);
    uint8_t HC_flag = get_bit(s->reg[RF], HALFCARRY_FLAG_BIT);
    uint8_t C_flag = get_bit(s->reg[RF], CARRY_FLAG_BIT);

    uint8_t new_C_flag = 0;
    if (sub_flag){
        uint8_t adj = (HC_flag * 0x6) + (C_flag * 0x60);
        new_C_flag=C_flag;
        s->reg[RA] -= adj;
    } else {
        uint8_t adj = 0;
        if (((s->reg[RA] & 0x0F) > 0x09) || HC_flag) adj += 0x06;
        if (((s->reg[RA] & 0xFF) > 0x99) || C_flag) new_C_flag=1, adj += 0x60;
        s->reg[RA] += adj;
    }

    set_flags(s, !(s->reg[RA]), LEAVE_BIT_AS_IS, 0, new_C_flag);
    return 1;
}

uint16_t jmp(uint8_t instr, gb_t *s){
    uint8_t bit_to_check = ((instr >> 4) & 0x01) ? CARRY_FLAG_BIT : ZERO_FLAG_BIT;
    uint8_t cond = !get_bit(s->reg[RF], bit_to_check);
    uint16_t new_addr = get_mem(s, s->pc+1);
    new_addr <<= 8;
    new_addr += get_mem(s, s->pc);
    s->pc+=2;
    if ((instr >> 3) & 0x01){
        cond = !cond;
    }
    if (cond || instr == 0xC3){
        s->pc = new_addr;
        return 4;
    } else {
        return 3;
    }

}
uint16_t di(uint8_t instr, gb_t *s){
    s->ime=0;
    return 1;
}

uint16_t ei(uint8_t instr, gb_t *s){
    s->ime=1;
    return 1;
}

uint16_t ld_hl_sp(uint8_t instr, gb_t *s){
    int8_t offset = get_mem(s, s->pc++);
    uint16_t tgt = s->sp + offset;
    s->reg[RH] = tgt >> 8;
    s->reg[RL] = tgt & 0x00FF;
    uint8_t carry;
    uint8_t halfcarry;
    carry = calc_add_carry_8(s->sp, offset, 0);
    halfcarry = calc_add_halfcarry_8(s->sp, offset, 0);
    set_flags(s, 0, 0, halfcarry, carry);
    return 3;
}


uint16_t ld_sp_hl(uint8_t instr, gb_t *s){
    uint16_t tgt = s->reg[RH];
    tgt <<= 8;
    tgt += s->reg[RL];
    s->sp = tgt;
    return 2;
}

uint16_t ld_ia(uint8_t instr, gb_t *s){
    uint16_t cycles;
    uint16_t addr;
    if (instr & 0x08) { // two-byte load immediate
        cycles = 4;
        addr = get_mem(s, s->pc+1);
        addr <<= 8;
        addr += get_mem(s, s->pc);
        s->pc+=2;
    } else {
        cycles = 3;
        addr = 0xFF00;
        addr += get_mem(s, s->pc++);
    }
    if (instr & 0x10) { // RA -> RAM
        s->reg[RA] = get_mem(s, addr);
    } else {
        set_mem(s, addr, s->reg[RA]);
    }
    return cycles;
}

uint16_t ld_rc_a(uint8_t instr, gb_t *s){
    uint16_t addr = 0xFF00 + s->reg[RC];
    if (instr & 0x10) { // RA -> RAM
        s->reg[RA] = get_mem(s, addr);
    } else {
        set_mem(s, addr, s->reg[RA]);
    }

    return 2;
}

uint16_t adi_sp(uint8_t instr, gb_t *s){
    int8_t offset = get_mem(s, s->pc++);
    uint8_t carry;
    uint8_t halfcarry;
    carry = calc_add_carry_8(s->sp, offset, 0);
    halfcarry = calc_add_halfcarry_8(s->sp, offset, 0);
    set_flags(s, 0, 0, halfcarry, carry);
    s->sp += offset;
    return 4;
}

uint16_t jp_hl(uint8_t instr, gb_t *s){
    uint16_t tgt = s->reg[RH];
    tgt <<= 8;
    tgt += s->reg[RL];
    s->pc = tgt;
    return 1;
}

uint16_t callcond(uint8_t instr, gb_t *s){
    uint8_t bit_to_check = ((instr >> 4) & 0x01) ? CARRY_FLAG_BIT : ZERO_FLAG_BIT;
    uint8_t cond = !get_bit(s->reg[RF], bit_to_check);
    uint16_t addr = get_mem(s, s->pc+1);
    addr <<= 8;
    addr += get_mem(s, s->pc);
    s->pc+=2;
    if ((instr >> 3) & 0x01){
        cond = !cond;
    }
    if (cond){
        _call(addr, s);
        return 6;
    } else {
        return 3;
    }
}

uint16_t call(uint8_t instr, gb_t *s){
    uint16_t addr = get_mem(s, s->pc+1);
    addr <<= 8;
    addr += get_mem(s, s->pc);
    s->pc+=2;
    _call(addr, s);
    return 6;
}

uint16_t jr(uint8_t instr, gb_t *s){
    uint16_t cond;
    int8_t offset = get_mem(s, s->pc++);
    switch((instr >> 4) & 0x03) {
        case 1:
            cond = 1; // unconditional JR
            break;
        case 2:
            cond = get_bit(s->reg[RF], ZERO_FLAG_BIT);
            break;
        case 3:
            cond = get_bit(s->reg[RF], CARRY_FLAG_BIT);
            break;
    }
    if (!(instr & 0x08)) {
        cond = !cond;
    }
    if (cond){
        s->pc += offset;
        return 3;
    } else {
        return 2;
    }
}

uint16_t stop(uint8_t instr, gb_t *s) {
    //s->pc++; // ???
    return 3; // 3 cycles???? documentation doesn't seem to agree on behavior here.
}

uint16_t ld_ia_sp(uint8_t instr, gb_t *s) {
    uint16_t addr = get_mem(s, s->pc+1);
    addr <<= 8;
    addr += get_mem(s, s->pc);
    s->pc+=2;
    set_mem(s, addr, s->sp & 0xFF);
    set_mem(s, addr+1, s->sp >> 8);
    return 5;
}
