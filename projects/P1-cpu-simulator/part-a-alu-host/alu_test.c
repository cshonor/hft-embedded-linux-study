#include "alu.h"

#include <stdio.h>

static int fails; /* 失败次数；最后用来当进程退出码 */

/* cond 为真就印 PASS，否则 FAIL。新手看测试：期望什么，对照 alu_exec 的输出。 */
static int expect(const char *name, int cond)
{
    if (cond) {
        printf("PASS  %s\n", name);
        return 0;
    }
    printf("FAIL  %s\n", name);
    ++fails;
    return 1;
}

int main(void)
{
    struct AluResult r;

    r = alu_exec(ALU_ADD, 1, 2);
    expect("add 1+2", r.value == 3 && !r.flags.z && !r.flags.c && !r.flags.v);

    /* 200+100=300 → 存 44，进位 C=1（无符号溢出，但不是有符号 V） */
    r = alu_exec(ALU_ADD, 200, 100);
    expect("add carry", r.value == 44 && r.flags.c && !r.flags.v);

    r = alu_exec(ALU_ADD, 0, 0);
    expect("add zero", r.value == 0 && r.flags.z);

    /* 127+1=128，有符号从最大正数翻成 -128，所以 V=1、N=1 */
    r = alu_exec(ALU_ADD, 0x7F, 1);
    expect("add signed overflow", r.value == 0x80 && r.flags.v && r.flags.n);

    r = alu_exec(ALU_SUB, 5, 3);
    expect("sub 5-3", r.value == 2 && !r.flags.c);

    /* 3-5 要借位：结果 0xFE（-2 的补码），C=1 */
    r = alu_exec(ALU_SUB, 3, 5);
    expect("sub borrow", r.value == 0xFE && r.flags.c && r.flags.n);

    /* -128 - 1 有符号溢出，变成 +127 */
    r = alu_exec(ALU_SUB, 0x80, 1);
    expect("sub signed overflow", r.value == 0x7F && r.flags.v);

    r = alu_exec(ALU_AND, 0xF0, 0x3C);
    expect("and", r.value == 0x30);

    r = alu_exec(ALU_OR, 0x0F, 0x30);
    expect("or", r.value == 0x3F);

    r = alu_exec(ALU_XOR, 0xFF, 0x0F);
    expect("xor", r.value == 0xF0);

    /* 1000_0001 左移 → 0000_0010，挤出的 1 进 C */
    r = alu_exec(ALU_SHL, 0x81, 0);
    expect("shl", r.value == 0x02 && r.flags.c);

    r = alu_exec(ALU_SHR, 0x03, 0);
    expect("shr", r.value == 0x01 && r.flags.c);

    r = alu_exec(ALU_PASS_A, 0x5A, 0xFF);
    expect("pass", r.value == 0x5A);

    printf("alu self-test %s (%d failed)\n", fails ? "FAILED" : "OK", fails);
    return fails ? 1 : 0; /* 给 make test / CI：0=成功 */
}
