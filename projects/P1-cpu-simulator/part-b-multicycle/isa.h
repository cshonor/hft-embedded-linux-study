#ifndef P1_ISA_H
#define P1_ISA_H

#include <stdint.h>

/*
 * 一条指令 16 位，存在 IMem 里。拆法：
 *
 *  15           12 11  10  9   8  7   6  5          0
 * +---------------+------+------+------+-------------+
 * |     opcode    |  rd  |  rs  |  rt  |   未用      |   R 型：ADD rd, rs, rt
 * +---------------+------+------+------+-------------+
 * |     opcode    |  rd  |      imm8（低 8 位）      |   I 型：MOVI rd, #imm
 * +---------------+------+---------------------------+
 *
 * rd/rs/rt 各 2 位 → 只能有 R0..R3 四个寄存器。
 * JNZ 的跳转目标也放在 imm8：那是指令地址（0,1,2...），不是字节地址。
 *
 * opcode 从 0 往上排，和下面 enum 顺序一致（MOVI=1, ADD=2, ...）。
 */
enum {
    OP_NOP = 0, /* 空操作，4 拍什么也不改 */
    OP_MOVI,    /* rd = imm */
    OP_ADD,     /* rd = rs + rt */
    OP_SUB,
    OP_AND,
    OP_OR,
    OP_LOAD,    /* rd = dmem[rs]        从数据内存读 */
    OP_STORE,   /* dmem[rs] = rt        写数据内存；rd 字段不用 */
    OP_JMP,     /* pc = imm             无条件跳 */
    OP_JZ,      /* 若 Z=1 则 pc = imm */
    OP_JNZ,     /* 若 Z=0 则 pc = imm   循环常用 */
    OP_ADDI,    /* rd = rd + imm */
    OP_SUBI,    /* rd = rd - imm        减到 0 时 Z=1，给 JNZ 用 */
    OP_MOV,     /* rd = rs */
    OP_CMP,     /* 做 rs-rt，只改标志，不写寄存器 */
    OP_HALT     /* 停机 */
};

#define R0 0
#define R1 1
#define R2 2
#define R3 3

/* 把字段「拼」进 16 位机器码。| 是按位或：各字段占不同位，互不覆盖。 */
static inline uint16_t enc_r(int op, int rd, int rs, int rt)
{
    return (uint16_t)((op << 12) | (rd << 10) | (rs << 8) | (rt << 6));
}

static inline uint16_t enc_i(int op, int rd, unsigned imm)
{
    return (uint16_t)((op << 12) | (rd << 10) | (imm & 0xFFu));
}

#endif
