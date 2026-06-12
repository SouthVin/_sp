#include "toyemu.h"
#include <stdio.h>
#include <string.h>

void cpu_init(CPU *cpu) {
    memset(cpu->reg, 0, sizeof(cpu->reg));
    cpu->pc      = 0;
    cpu->sp      = STACK_INIT;
    cpu->flags   = 0;
    cpu->running = 1;
    memset(cpu->memory, 0, sizeof(cpu->memory));
}

void cpu_load(CPU *cpu, const uint8_t *program, size_t size) {
    memcpy(cpu->memory, program, size < MEM_SIZE ? size : MEM_SIZE);
}

static void set_flags(CPU *cpu, uint16_t result) {
    cpu->flags = 0;
    if ((result & 0xFF) == 0) cpu->flags |= FLAG_Z;
    if (result > 0xFF)        cpu->flags |= FLAG_C;
}

static uint8_t fetch(CPU *cpu) {
    return cpu->memory[cpu->pc++];
}

int cpu_step(CPU *cpu) {
    if (!cpu->running) return 0;

    uint8_t opcode = fetch(cpu);
    uint8_t b1, b2, rd, rs, addr;
    uint16_t tmp;

    switch (opcode) {
    case OP_NOP:
        break;
    case OP_HLT:
        cpu->running = 0;
        break;
    case OP_MOV:
        b1 = fetch(cpu);
        rd = b1 >> 4;
        rs = b1 & 0x0F;
        cpu->reg[rd] = cpu->reg[rs];
        break;
    case OP_MVI:
        rd = fetch(cpu);
        b2 = fetch(cpu);
        cpu->reg[rd] = b2;
        break;
    case OP_LD:
        rd   = fetch(cpu);
        addr = fetch(cpu);
        cpu->reg[rd] = cpu->memory[addr];
        break;
    case OP_ST:
        rs   = fetch(cpu);
        addr = fetch(cpu);
        cpu->memory[addr] = cpu->reg[rs];
        break;
    case OP_ADD:
        b1 = fetch(cpu);
        rd = b1 >> 4;
        rs = b1 & 0x0F;
        tmp = (uint16_t)cpu->reg[rd] + cpu->reg[rs];
        set_flags(cpu, tmp);
        cpu->reg[rd] = (uint8_t)tmp;
        break;
    case OP_SUB:
        b1 = fetch(cpu);
        rd = b1 >> 4;
        rs = b1 & 0x0F;
        tmp = (uint16_t)cpu->reg[rd] - cpu->reg[rs];
        set_flags(cpu, tmp);
        cpu->reg[rd] = (uint8_t)tmp;
        break;
    case OP_CMP:
        b1 = fetch(cpu);
        rd = b1 >> 4;
        rs = b1 & 0x0F;
        tmp = (uint16_t)cpu->reg[rd] - cpu->reg[rs];
        set_flags(cpu, tmp);
        break;
    case OP_INC:
        rd = fetch(cpu);
        tmp = (uint16_t)cpu->reg[rd] + 1;
        set_flags(cpu, tmp);
        cpu->reg[rd] = (uint8_t)tmp;
        break;
    case OP_DEC:
        rd = fetch(cpu);
        tmp = (uint16_t)cpu->reg[rd] - 1;
        set_flags(cpu, tmp);
        cpu->reg[rd] = (uint8_t)tmp;
        break;
    case OP_AND:
        b1 = fetch(cpu);
        rd = b1 >> 4;
        rs = b1 & 0x0F;
        cpu->reg[rd] &= cpu->reg[rs];
        cpu->flags = (cpu->reg[rd] == 0) ? FLAG_Z : 0;
        break;
    case OP_OR:
        b1 = fetch(cpu);
        rd = b1 >> 4;
        rs = b1 & 0x0F;
        cpu->reg[rd] |= cpu->reg[rs];
        cpu->flags = (cpu->reg[rd] == 0) ? FLAG_Z : 0;
        break;
    case OP_XOR:
        b1 = fetch(cpu);
        rd = b1 >> 4;
        rs = b1 & 0x0F;
        cpu->reg[rd] ^= cpu->reg[rs];
        cpu->flags = (cpu->reg[rd] == 0) ? FLAG_Z : 0;
        break;
    case OP_JMP:
        addr = fetch(cpu);
        cpu->pc = addr;
        break;
    case OP_JZ:
        addr = fetch(cpu);
        if (cpu->flags & FLAG_Z) cpu->pc = addr;
        break;
    case OP_JNZ:
        addr = fetch(cpu);
        if (!(cpu->flags & FLAG_Z)) cpu->pc = addr;
        break;
    case OP_CALL:
        addr = fetch(cpu);
        cpu->memory[cpu->sp--] = cpu->pc;
        cpu->pc = addr;
        break;
    case OP_RET:
        cpu->pc = cpu->memory[++cpu->sp];
        break;
    case OP_PUSH:
        rs = fetch(cpu);
        cpu->memory[cpu->sp--] = cpu->reg[rs];
        break;
    case OP_POP:
        rd = fetch(cpu);
        cpu->reg[rd] = cpu->memory[++cpu->sp];
        break;
    case OP_OUT:
        rd = fetch(cpu);
        printf("[OUT] R%d = %d\n", rd, cpu->reg[rd]);
        break;
    case OP_OUTC:
        rd = fetch(cpu);
        putchar((char)cpu->reg[rd]);
        fflush(stdout);
        break;
    default:
        fprintf(stderr, "Illegal opcode 0x%02X at PC=0x%02X\n", opcode, cpu->pc - 1);
        cpu->running = 0;
        return -1;
    }
    return 1;
}

void cpu_run(CPU *cpu) {
    int step_limit = 10000;
    while (cpu->running && step_limit-- > 0) {
        if (cpu_step(cpu) < 0) break;
    }
    if (cpu->running) {
        fprintf(stderr, "[CPU] Step limit reached (possible infinite loop)\n");
        cpu->running = 0;
    }
}

const char *reg_name(int r) {
    static const char *names[] = {"R0", "R1", "R2", "R3"};
    return (r >= 0 && r < NUM_REGS) ? names[r] : "??";
}
