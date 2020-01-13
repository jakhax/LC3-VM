# LC-3 Virtual Machine
- This is a virtual machine that can run assembly language programs for the [lc3](https://en.wikipedia.org/wiki/Little_Computer_3) computer. The Virtual machine is simple and the main aim to learn computer architecture, but theoretically its even capable of running an operating system !
>> A VM is a program that acts like a computer. It simulates a CPU along with a few other hardware components, allowing it to perform arithmetic, read and write to memory, and interact with I/O devices, just like a physical computer. Most importantly, it can understand a machine language which you can use to program it.
- The aim is to understand how VMs (like the Java Virtual Machine which runs Java, Kotlin, or Clojure programs) work and build our own simple VM.

## Resources
- [Write your Own Virtual Machine](https://justinmeiners.github.io/lc3-vm). Super helpful step by step guide. 
- [LC-3 Simulator](https://wchargin.github.io/lc3web/)
- [https://justinmeiners.github.io/lc3-vm/supplies/lc3-isa.pdf](https://justinmeiners.github.io/lc3-vm/supplies/lc3-isa.pdf). Reference for the LC-3 architecture.

### Build
```
gcc main.c -o lc3
```

## LC-3 architecture
- This VM simulates a fictional computer, [lc3](https://en.wikipedia.org/wiki/Little_Computer_3) used to teach assembly language.

### Memory
- The LC-3 has 65,536 memory locations, `2^16`, each store a 16 bit value.
```c
uint16_t memory[UINT16_MAX];
```
### Registers
- The LC-3 has 10 registers each 16 bits. Most are general purpose.
    - 8 general purpose registers
    - 1 program counter (PC)
    - 1 conditional register (COND)

```c
enum
{
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
    R_COUNT
};
uint16_t registers[R_COUNT];
```
### Instruction set
- Instruction tell the lc3 cpu to do some task, each 16 bit intruction  has an opcode, 4 left bits which indicate some of task to perform on a set of parameters, represented by the rest of the bits. LC-3 has 16 opcodes.
```c
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
```

### Procedure
1. Load one instruction from memory at the address of the PC register.
2. Increment the PC register.
3. Look at the opcode to determine which type of instruction it should perform.
4. Perform the instruction using the parameters in the instruction.
5. Go back to step 1.

### Usage
```bash
./lc3 <program>
#example
./lc3 examples/hello-world
```
- Below is an `hello-word` program for the `lc3`, you'll can find the assembled program in `example_programs` directory
```shell
.ORIG x3000                        ; this is the address in memory where the program will be loaded
LEA R0, HELLO_STR                  ; load the address of the HELLO_STR string into R0
PUTs                               ; output the string pointed to by R0 to the console
HALT                               ; halt the program
HELLO_STR .STRINGZ "Hello World!"  ; store this string here in the program
.END                               ; mark the end of the file
```

