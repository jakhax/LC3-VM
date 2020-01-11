#include <stdint.h>

// memory
uint16_t memory[UINT16_MAX];

// registers

enum {
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC,
    R_COND,
    R_COUNT,
};

uint16_t reg[R_COUNT];

enum
{
    OP_BR = 0, /* branch */
    OP_ADD,    /* add  */
    OP_LD,     /* load */
    OP_ST,     /* store */
    OP_JSR,    /* jump register */
    OP_AND,    /* bitwise and */
    OP_LDR,    /* load register */
    OP_STR,    /* store register */
    OP_RTI,    /* unused */
    OP_NOT,    /* bitwise not */
    OP_LDI,    /* load indirect */
    OP_STI,    /* store indirect */
    OP_JMP,    /* jump */
    OP_RES,    /* reserved (unused) */
    OP_LEA,    /* load effective address */
    OP_TRAP    /* execute trap */
};

// conditional flags
enum {
    FLAG_POS = 1 << 0,
    FLAG_ZRO = 1 << 1,
    FLAG_NEG = 1 << 2, 
};

void update_flags(uint16_t r0){
    if(reg[r0]>>15 == 1){
        reg[R_COND] = FLAG_NEG;
    }else if(reg[r0] == 0){
        reg[R_COND] = FLAG_ZRO;
    }else{
        reg[R_COND] = FLAG_POS;
    }
}

uint16_t sign_extend(uint16_t x, int bit_count)
{
    if ((x >> (bit_count - 1)) & 1) {
        x |= (0xFFFF << bit_count);
    }
    return x;
}

void op_add(uint16_t instr){
    // register to store results
    uint16_t dr = (instr >> 9) & 0x7;
    //sr1 register
    uint16_t sr1 = (instr >> 6) & 0x7;
    //imm flag
    uint16_t imm_flag = (instr >> 5) & 0x1;
    if(imm_flag){
        uint16_t imm = instr  & 0x1f;
        reg[dr] = reg[sr1] + sign_extend(imm,5);
    }else{
        uint16_t sr2 =  instr & 0x7;
        reg[dr] = reg[sr1] + reg[sr2];
    }
    update_flags(dr);
}

void op_and(uint16_t instr){
    uint16_t dr = (instr >> 9) & 0x7;
    uint16_t sr1 = (instr >> 6) & 0x7;
    uint16_t imm_flag = (instr >> 5) & 0x1;
    if (imm_flag){
        uint16_t imm = instr & 0x1f;
        reg[dr] = reg[sr1] & sign_extend(imm,5);
    }else{
        reg[dr] = reg[sr1] & reg[(instr & 0x7)];
    } 
    update_flags(dr);
}

uint16_t op_br(uint16_t ins){
    uint16_t f = (ins >> 9) & 0x7;
    uint16_t pc_offset = sign_extend((ins & 0x1ff),9);
    if(f & reg[R_COND]){
        reg[R_PC] += pc_offset;
    }
}

uint16_t op_jmp(uint16_t ins){
    uint16_t r =  (ins >> 6) & 0x7;
    reg[R_PC] = reg[r];
}

uint16_t op_jsr(uint16_t ins){
    reg[R_R7] = reg[R_PC];
    uint16_t f = (ins >> 11) & 1;
    if(f){
        // jsr
        reg[R_PC] += sign_extend((ins & 0x7ff),11);
    }else{
        // jssr
        reg[R_PC] = (ins >> 6) & 0x7;
    }
}

uint16_t op_ld(uint16_t ins){
    uint16_t dr = (ins >> 9) & 0x7;
    uint16_t addr = sign_extend((ins & 0x1ff),9) + reg[R_PC];
    reg[dr] = mem_read(addr);
    update_flags(dr);
}

uint16_t op_ldi(uint16_t ins){
    uint16_t dr = (ins >> 9) & 0x7;
    uint16_t addr = sign_extend((ins & 0x1ff),9) + reg[R_PC];
    reg[dr] = mem_read(mem_read(addr));
    update_flags(dr);
}

uint16_t op_ldr(uint16_t ins){
    uint16_t dr = (ins >> 9) & 0x7;
    uint16_t addr = reg[(ins>>6) & 0x7] + sign_extend(ins & 0x3f,6);
    reg[dr] = mem_read(addr);
    update_flags(dr);
}

uint16_t op_lea(uint16_t ins){
    uint16_t dr = (ins >> 9) & 0x7;
    uint16_t addr = sign_extend(ins & 0x1ff,9) + reg[R_PC];
    reg[dr] = addr;
    update_flags(dr);
}

uint16_t op_not(uint16_t ins){
    uint16_t dr = (ins >> 9) & 0x7;
    uint16_t sr = (ins >> 6) & 0x7;
    reg[dr] = ~reg[sr];
    update_flags(dr);
}

uint16_t op_st(uint16_t ins){
    uint16_t sr = (ins >> 9) & 0x7;
    uint16_t addr = sign_extend(ins & 0x1ff, 9) + reg[R_PC];
    uint16_t val = reg[sr];
    mem_write(addr,val);

}

uint16_t mem_read(uint16_t addr){
    // todo implement mem read
    uint16_t val = memory[addr];
    return val;
}

void mem_write(uint16_t addr, uint16_t val){
    memory[addr] = val;
}

int main(){
    uint16_t instr = mem_read(reg[R_PC]++);
    uint16_t op = instr >> 12;
    switch (op)
    {
    case OP_ADD:
        op_add(instr);
        break;
    case OP_AND:
        op_and(instr);
        break;
    case OP_BR:
        op_br(instr);
        break;
    case OP_JMP:
        op_jmp(instr);
        break;
    case OP_JSR:
        op_jsr(instr);
        break;
    case OP_LD:
        op_ld(instr);
        break;
    case OP_LDI:
        op_ldi(instr);
        break;
    case OP_LDR:
        op_ldr(instr);
        break;
    case OP_LEA:
        op_lea(instr);
        break;
    case OP_NOT:
        op_not(instr);
        break;
    case OP_ST:
        op_st(instr);
        break;
    case OP_RES:
    case OP_RTI:
    default:
        abort();
        break;
    }
    return 0;
}

