#ifndef P1_ALU_H
#define P1_ALU_H

#include <stdint.h>

/* uint8_t = 无符号 8 位，范围 0..255。CPU 里一个「字节」就是这个宽度。 */

enum AluOp {
    ALU_ADD = 0, /* a + b */
    ALU_SUB,     /* a - b */
    ALU_AND,     /* 按位与 */
    ALU_OR,      /* 按位或 */
    ALU_XOR,     /* 按位异或 */
    ALU_SHL,     /* 左移 1 位，空位补 0 */
    ALU_SHR,     /* 右移 1 位，空位补 0（逻辑移位） */
    ALU_PASS_A   /* 结果 = a，MOV 会用到：不运算，只把数传过去 */
};

/*
 * 四个标志位：ALU 算完后「顺便」告诉你结果长什么样。
 * 后面 JZ / JNZ 就靠 Z 决定跳不跳。对应笔记 00 ch5.2。
 *
 * Z (Zero)     结果是不是 0
 * N (Negative) 把 bit7 当符号位时，结果是不是负数（1 = 负数）
 * C (Carry)    加法：有没有进位出第 8 位；减法：有没有借位
 * V (oVerflow) 当成有符号数（-128..127）时有没有溢出
 *
 * C 和 V 不是一回事：200+100 无符号溢出（C=1），但当有符号看不一定 V=1。
 */
struct AluFlags {
    int z;
    int n;
    int c;
    int v;
};

struct AluResult {
    uint8_t value;         /* 8 位结果（超过 255 的部分被丢掉） */
    struct AluFlags flags; /* 同时算出的标志 */
};

struct AluResult alu_exec(enum AluOp op, uint8_t a, uint8_t b);

#endif
