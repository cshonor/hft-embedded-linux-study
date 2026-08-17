#include "cpu.h"

#include <stdio.h>
#include <string.h>

static const char *state_name(enum CpuState s)
{
    switch (s) {
    case ST_FETCH:     return "FETCH";
    case ST_DECODE:    return "DECODE";
    case ST_EXECUTE:   return "EXECUTE";
    case ST_WRITEBACK: return "WB";
    case ST_HALT:      return "HALT";
    }
    return "?";
}

void cpu_reset(struct Cpu *c)
{
    memset(c, 0, sizeof *c);
    c->state = ST_FETCH;
}

static void apply_flags(struct Cpu *c, struct AluResult res)
{
    c->flags = res.flags;
}

static void dump(const struct Cpu *c)
{
    if (!c->trace) {
        return;
    }
    printf("cyc=%-4d %-8s pc=%02x ir=%04x r=[%02x %02x %02x %02x] Z=%d C=%d N=%d V=%d\n",
           c->cycles,
           state_name(c->state),
           c->pc,
           c->ir,
           c->r[0], c->r[1], c->r[2], c->r[3],
           c->flags.z, c->flags.c, c->flags.n, c->flags.v);
}

void cpu_clock(struct Cpu *c)
{
    struct AluResult res;

    if (c->halted) {
        return;
    }

    dump(c);
    c->cycles++;

    switch (c->state) {
    case ST_FETCH:
        c->ir = c->imem[c->pc];
        c->pc = (uint8_t)(c->pc + 1);
        c->state = ST_DECODE;
        break;

    case ST_DECODE:
        c->op  = (c->ir >> 12) & 0xF;
        c->rd  = (c->ir >> 10) & 0x3;
        c->rs  = (c->ir >> 8) & 0x3;
        c->rt  = (c->ir >> 6) & 0x3;
        c->imm = (uint8_t)(c->ir & 0xFF);
        c->reg_we = 0;
        if (c->op == OP_HALT) {
            c->state = ST_HALT;
            c->halted = 1;
            break;
        }
        c->state = ST_EXECUTE;
        break;

    case ST_EXECUTE:
        switch (c->op) {
        case OP_NOP:
            break;
        case OP_MOVI:
            c->wb_val = c->imm;
            c->reg_we = 1;
            break;
        case OP_MOV:
            c->wb_val = c->r[c->rs];
            c->reg_we = 1;
            break;
        case OP_ADD:
            res = alu_exec(ALU_ADD, c->r[c->rs], c->r[c->rt]);
            apply_flags(c, res);
            c->wb_val = res.value;
            c->reg_we = 1;
            break;
        case OP_SUB:
            res = alu_exec(ALU_SUB, c->r[c->rs], c->r[c->rt]);
            apply_flags(c, res);
            c->wb_val = res.value;
            c->reg_we = 1;
            break;
        case OP_AND:
            res = alu_exec(ALU_AND, c->r[c->rs], c->r[c->rt]);
            apply_flags(c, res);
            c->wb_val = res.value;
            c->reg_we = 1;
            break;
        case OP_OR:
            res = alu_exec(ALU_OR, c->r[c->rs], c->r[c->rt]);
            apply_flags(c, res);
            c->wb_val = res.value;
            c->reg_we = 1;
            break;
        case OP_ADDI:
            res = alu_exec(ALU_ADD, c->r[c->rd], c->imm);
            apply_flags(c, res);
            c->wb_val = res.value;
            c->reg_we = 1;
            break;
        case OP_SUBI:
            res = alu_exec(ALU_SUB, c->r[c->rd], c->imm);
            apply_flags(c, res);
            c->wb_val = res.value;
            c->reg_we = 1;
            break;
        case OP_CMP:
            res = alu_exec(ALU_SUB, c->r[c->rs], c->r[c->rt]);
            apply_flags(c, res);
            break;
        case OP_LOAD:
            c->wb_val = c->dmem[c->r[c->rs]];
            c->reg_we = 1;
            break;
        case OP_STORE:
            c->dmem[c->r[c->rs]] = c->r[c->rt];
            break;
        case OP_JMP:
            c->pc = c->imm;
            break;
        case OP_JZ:
            if (c->flags.z) {
                c->pc = c->imm;
            }
            break;
        case OP_JNZ:
            if (!c->flags.z) {
                c->pc = c->imm;
            }
            break;
        default:
            break;
        }
        c->state = ST_WRITEBACK;
        break;

    case ST_WRITEBACK:
        if (c->reg_we) {
            c->r[c->rd] = c->wb_val;
        }
        c->state = ST_FETCH;
        break;

    case ST_HALT:
        c->halted = 1;
        break;
    }
}

int cpu_run(struct Cpu *c, int max_cycles)
{
    while (!c->halted && c->cycles < max_cycles) {
        cpu_clock(c);
    }
    return c->halted;
}

void load_sum(struct Cpu *c)
{
    cpu_reset(c);
    c->imem[0] = enc_i(OP_MOVI, R0, 0);
    c->imem[1] = enc_i(OP_MOVI, R1, 5);
    c->imem[2] = enc_r(OP_ADD, R0, R0, R1);
    c->imem[3] = enc_i(OP_SUBI, R1, 1);
    c->imem[4] = enc_i(OP_JNZ, 0, 2);
    c->imem[5] = enc_i(OP_HALT, 0, 0);
}

void load_fib(struct Cpu *c)
{
    cpu_reset(c);
    c->imem[0] = enc_i(OP_MOVI, R0, 0);
    c->imem[1] = enc_i(OP_MOVI, R1, 1);
    c->imem[2] = enc_i(OP_MOVI, R2, 6);
    c->imem[3] = enc_r(OP_ADD, R3, R0, R1);
    c->imem[4] = enc_r(OP_MOV, R0, R1, 0);
    c->imem[5] = enc_r(OP_MOV, R1, R3, 0);
    c->imem[6] = enc_i(OP_SUBI, R2, 1);
    c->imem[7] = enc_i(OP_JNZ, 0, 3);
    c->imem[8] = enc_i(OP_HALT, 0, 0);
}

void load_memcpy(struct Cpu *c)
{
    cpu_reset(c);
    c->dmem[0x10] = 0xAA;
    c->dmem[0x11] = 0xBB;
    c->dmem[0x12] = 0xCC;
    c->dmem[0x13] = 0xDD;
    c->imem[0] = enc_i(OP_MOVI, R0, 0x10);
    c->imem[1] = enc_i(OP_MOVI, R1, 0x20);
    c->imem[2] = enc_i(OP_MOVI, R2, 4);
    c->imem[3] = enc_r(OP_LOAD, R3, R0, 0);
    c->imem[4] = enc_r(OP_STORE, 0, R1, R3);
    c->imem[5] = enc_i(OP_ADDI, R0, 1);
    c->imem[6] = enc_i(OP_ADDI, R1, 1);
    c->imem[7] = enc_i(OP_SUBI, R2, 1);
    c->imem[8] = enc_i(OP_JNZ, 0, 3);
    c->imem[9] = enc_i(OP_HALT, 0, 0);
}
