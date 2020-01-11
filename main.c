#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/termios.h>
#include <signal.h>

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

enum
{
    TRAP_GETC = 0x20,  /* get character from keyboard, not echoed onto the terminal */
    TRAP_OUT = 0x21,   /* output a character */
    TRAP_PUTS = 0x22,  /* output a word string */
    TRAP_IN = 0x23,    /* get character from keyboard, echoed onto the terminal */
    TRAP_PUTSP = 0x24, /* output a byte string */
    TRAP_HALT = 0x25   /* halt the program */
};

enum
{
    MR_KBSR = 0xFE00, /* keyboard status */
    MR_KBDR = 0xFE02  /* keyboard data */
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

uint16_t swap16(uint16_t x)
{
    return (x << 8) | (x >> 8);
}

uint16_t check_key()
{
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    return select(1, &readfds, NULL, NULL, &timeout) != 0;
}


uint16_t mem_read(uint16_t addr){
    if(addr == MR_KBSR){
        if(check_key()){
            memory[MR_KBSR] = 1 << 15;
            memory[MR_KBDR] = getchar();
        }else{
            memory[MR_KBSR] = 0;
        }
    }
    uint16_t val = memory[addr];
    return val;
}

void mem_write(uint16_t addr, uint16_t val){
    memory[addr] = val;
}

int load_program_from_file(const char* file){
    FILE* image= fopen(file,"rb");
    if(!image){
        return 0;
    }
    uint16_t origin;
    fread(&origin,sizeof(origin),1,image);
    origin = swap16(origin);
    uint16_t max_read = UINT16_MAX - origin;
    uint16_t* i = memory + origin;
    size_t read = fread(i,sizeof(uint16_t),max_read,image);
    // swap
    while(read-- > 0){
        *i = swap16(*i);
        ++i;
    }
    return 1;
}

void op_add(uint16_t instr){
    // rister to store results
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

void op_br(uint16_t ins){
    uint16_t f = (ins >> 9) & 0x7;
    uint16_t pc_offset = sign_extend((ins & 0x1ff),9);
    if(f & reg[R_COND]){
        reg[R_PC] += pc_offset;
    }
}

void op_jmp(uint16_t ins){
    uint16_t r =  (ins >> 6) & 0x7;
    reg[R_PC] = reg[r];
}

void op_jsr(uint16_t ins){
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

void op_ld(uint16_t ins){
    uint16_t dr = (ins >> 9) & 0x7;
    uint16_t addr = sign_extend((ins & 0x1ff),9) + reg[R_PC];
    reg[dr] = mem_read(addr);
    update_flags(dr);
}

void op_ldi(uint16_t ins){
    uint16_t dr = (ins >> 9) & 0x7;
    uint16_t addr = sign_extend((ins & 0x1ff),9) + reg[R_PC];
    reg[dr] = mem_read(mem_read(addr));
    update_flags(dr);
}

void op_ldr(uint16_t ins){
    uint16_t dr = (ins >> 9) & 0x7;
    uint16_t addr = reg[(ins>>6) & 0x7] + sign_extend(ins & 0x3f,6);
    reg[dr] = mem_read(addr);
    update_flags(dr);
}

void op_lea(uint16_t ins){
    uint16_t dr = (ins >> 9) & 0x7;
    uint16_t addr = sign_extend(ins & 0x1ff,9) + reg[R_PC];
    reg[dr] = addr;
    update_flags(dr);
}

void op_not(uint16_t ins){
    uint16_t dr = (ins >> 9) & 0x7;
    uint16_t sr = (ins >> 6) & 0x7;
    reg[dr] = ~reg[sr];
    update_flags(dr);
}

void op_st(uint16_t ins){
    uint16_t sr = (ins >> 9) & 0x7;
    uint16_t addr = sign_extend(ins & 0x1ff, 9) + reg[R_PC];
    uint16_t val = reg[sr];
    mem_write(addr,val);

}

void op_sti(uint16_t ins){
    uint16_t sr = (ins >> 9) & 0x7;
    uint16_t addr = sign_extend(ins & 0x1ff, 9) + reg[R_PC];
    uint16_t val = reg[sr];
    mem_write(mem_read(addr),val);
}

void op_str(uint16_t ins){
    uint16_t sr = (ins >> 9) & 0x7;
    uint16_t br = (ins >> 6) & 0x7;
    uint16_t addr = reg[br] + sign_extend(ins & 0x3f,6);
    mem_write(addr, reg[sr]);
}

void trap_puts(){
    uint16_t* c = memory + reg[R_R0];
    while(*c){
        putc((char)*c,stdout);
        ++c;
    }
    fflush(stdout);
}

void trap_getc(){
    uint16_t c = (uint16_t)getchar();
    reg[R_R0] = c;
}

void trap_out(){
    putc((char)reg[R_R0],stdout);
    fflush(stdout);
}

void trap_putsp(){
    // 2 chars, in big endian c1 0:8,c2 9:15
    uint16_t* c = memory + reg[R_R0];
    while(*c){
        // c1
        putc((char)((*c)&0xff),stdout);
        // c2
        char c2 = (*c) >> 8;
        if(c2){
            putc(c2,stdout);
        }
        ++c;
    } 
    fflush(stdout);
}


/* Input Buffering */
struct termios original_tio;

void disable_input_buffering()
{
    tcgetattr(STDIN_FILENO, &original_tio);
    struct termios new_tio = original_tio;
    new_tio.c_lflag &= ~ICANON & ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}

void restore_input_buffering()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
}

void handle_interrupt(int signal)
{
    restore_input_buffering();
    printf("\n");
    exit(-2);
}

void setup(){
    signal(SIGINT, handle_interrupt);
    disable_input_buffering();
}


int main(int argc, const char* argv[]){
    if(argc != 2){
        printf("Usage: ./lc3 <program>");
        exit(2);
    }
    if(!load_program_from_file(argv[1])){
        printf("Unable to load program from file %s",argv[2]);
        exit(2);
    }

    setup();
    int running = 1;
    while (running)
    {
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
        case OP_STI:
            op_sti(instr);
            break;
        case OP_STR:
            op_str(instr);
            break;
        case OP_TRAP:
            switch(instr & 0xff){
                case TRAP_PUTS:
                    trap_puts();
                    break;
                case TRAP_GETC:
                    trap_getc();
                    break;
                case TRAP_OUT:
                    trap_out();
                    break;
                case TRAP_PUTSP:
                    trap_putsp();
                    break;
                case TRAP_HALT:
                    puts("HALTING");
                    fflush(stdout);
                    running = 0;
                    break;
            }
        case OP_RES:
        case OP_RTI:
        default:
            abort();
            break;
        }
    }
    restore_input_buffering();
    return 0;
}

