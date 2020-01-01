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

int main(){
    return -1;
}

