#ifndef TOYEMU_H
#define TOYEMU_H

#include <stdint.h>
#include <stddef.h>

#define MEM_SIZE    256
#define NUM_REGS    4
#define STACK_INIT  0xFF

enum {
    OP_NOP   = 0x00,
    OP_HLT   = 0x01,
    OP_MOV   = 0x02,
    OP_MVI   = 0x03,
    OP_LD    = 0x04,
    OP_ST    = 0x05,
    OP_ADD   = 0x06,
    OP_SUB   = 0x07,
    OP_CMP   = 0x08,
    OP_INC   = 0x09,
    OP_DEC   = 0x0A,
    OP_AND   = 0x0B,
    OP_OR    = 0x0C,
    OP_XOR   = 0x0D,
    OP_JMP   = 0x0E,
    OP_JZ    = 0x0F,
    OP_JNZ   = 0x10,
    OP_CALL  = 0x11,
    OP_RET   = 0x12,
    OP_PUSH  = 0x13,
    OP_POP   = 0x14,
    OP_OUT   = 0x15,
    OP_OUTC  = 0x16,
};

enum {
    FLAG_Z = 0x01,
    FLAG_C = 0x02,
};

typedef struct {
    uint8_t reg[NUM_REGS];
    uint8_t pc;
    uint8_t sp;
    uint8_t flags;
    uint8_t memory[MEM_SIZE];
    int     running;
} CPU;

void cpu_init(CPU *cpu);
void cpu_load(CPU *cpu, const uint8_t *program, size_t size);
int  cpu_step(CPU *cpu);
void cpu_run(CPU *cpu);

const char *reg_name(int r);

#endif
