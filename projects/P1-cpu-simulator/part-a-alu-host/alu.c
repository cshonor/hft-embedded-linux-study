#include "alu.h"

/* 根据结果 r 填 Z/N；C/V 由具体运算自己算好再传进来。 */
static struct AluFlags flags_from(uint8_t r, int c, int v)
{
    struct AluFlags f;
    f.z = (r == 0);
    f.n = (r >> 7) & 1; /* 只看最高位：1 表示「有符号负数」 */
    f.c = c;
    f.v = v;
    return f;
}

struct AluResult alu_exec(enum AluOp op, uint8_t a, uint8_t b)
{
    struct AluResult out;
    unsigned sum; /* 用比 8 位更宽的类型，才能看见「进位出去」的那一位 */
    uint8_t r = 0;
    int c = 0;
    int v = 0;

    switch (op) {
    case ALU_ADD:
        /* 例：200+100=300，8 位装不下。value 变成 300-256=44，C=1。 */
        sum = (unsigned)a + (unsigned)b;
        r = (uint8_t)sum;          /* 强转回 8 位 = 丢掉进位，只留低 8 位 */
        c = (sum > 0xFFu);         /* 0xFF=255；大于 255 说明进位了 */
        /* V：两个正数加出负数，或两个负数加出正数。
         * ~(a^b) 表示 a、b 符号相同；再和 (a^r) 合起来表示结果符号变了。 */
        v = ((~(a ^ b) & (a ^ r)) >> 7) & 1;
        break;
    case ALU_SUB:
        r = (uint8_t)(a - b); /* C 里减法也会回绕：3-5 → 254 */
        c = (a < b);          /* 无符号不够减 = 借位 */
        /* V：正减负变负，或负减正变正 */
        v = (((a ^ b) & (a ^ r)) >> 7) & 1;
        break;
    case ALU_AND:
        r = a & b;
        break;
    case ALU_OR:
        r = a | b;
        break;
    case ALU_XOR:
        r = a ^ b;
        break;
    case ALU_SHL:
        c = (a >> 7) & 1; /* 被挤出去的那一位放进 C */
        r = (uint8_t)(a << 1);
        break;
    case ALU_SHR:
        c = a & 1; /* 被挤出去的最低位 */
        r = (uint8_t)(a >> 1);
        break;
    case ALU_PASS_A:
        r = a; /* 忽略 b */
        break;
    }

    out.value = r;
    out.flags = flags_from(r, c, v);
    return out;
}
