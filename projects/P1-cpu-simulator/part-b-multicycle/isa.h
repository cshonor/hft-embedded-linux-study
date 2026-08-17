#ifndef P1_ISA_H
#define P1_ISA_H

#include <stdint.h>

enum {
    OP_NOP = 0,
    OP_MOVI,
    OP_ADD,
    OP_SUB,
    OP_AND,
    OP_OR,
    OP_LOAD,
    OP_STORE,
    OP_JMP,
    OP_JZ,
    OP_JNZ,
    OP_ADDI,
    OP_SUBI,
    OP_MOV,
    OP_CMP,
    OP_HALT
};

#define R0 0
#define R1 1
#define R2 2
#define R3 3

static inline uint16_t enc_r(int op, int rd, int rs, int rt)
{
    return (uint16_t)((op << 12) | (rd << 10) | (rs << 8) | (rt << 6));
}

static inline uint16_t enc_i(int op, int rd, unsigned imm)
{
    return (uint16_t)((op << 12) | (rd << 10) | (imm & 0xFFu));
}

#endif
