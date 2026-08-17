#include "alu.h"

static struct AluFlags flags_from(uint8_t r, int c, int v)
{
    struct AluFlags f;
    f.z = (r == 0);
    f.n = (r >> 7) & 1;
    f.c = c;
    f.v = v;
    return f;
}

struct AluResult alu_exec(enum AluOp op, uint8_t a, uint8_t b)
{
    struct AluResult out;
    unsigned sum;
    uint8_t r = 0;
    int c = 0;
    int v = 0;

    switch (op) {
    case ALU_ADD:
        sum = (unsigned)a + (unsigned)b;
        r = (uint8_t)sum;
        c = (sum > 0xFFu);
        v = ((~(a ^ b) & (a ^ r)) >> 7) & 1;
        break;
    case ALU_SUB:
        r = (uint8_t)(a - b);
        c = (a < b); /* 借位 */
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
        c = (a >> 7) & 1;
        r = (uint8_t)(a << 1);
        break;
    case ALU_SHR:
        c = a & 1;
        r = (uint8_t)(a >> 1);
        break;
    case ALU_PASS_A:
        r = a;
        break;
    }

    out.value = r;
    out.flags = flags_from(r, c, v);
    return out;
}
