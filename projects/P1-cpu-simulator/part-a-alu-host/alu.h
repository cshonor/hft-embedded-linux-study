#ifndef P1_ALU_H
#define P1_ALU_H

#include <stdint.h>

enum AluOp {
    ALU_ADD = 0,
    ALU_SUB,
    ALU_AND,
    ALU_OR,
    ALU_XOR,
    ALU_SHL,
    ALU_SHR,
    ALU_PASS_A
};

struct AluFlags {
    int z;
    int n;
    int c;
    int v;
};

struct AluResult {
    uint8_t value;
    struct AluFlags flags;
};

/* 8-bit ALU。C=无符号进位/借位，V=有符号溢出。对应 00 ch5.2。 */
struct AluResult alu_exec(enum AluOp op, uint8_t a, uint8_t b);

#endif
