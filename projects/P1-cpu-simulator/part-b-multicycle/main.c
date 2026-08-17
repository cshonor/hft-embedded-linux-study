#include "cpu.h"

#include <stdio.h>
#include <string.h>

/*
 * 把三个小程序跑一遍，检查寄存器/内存是不是预期值。
 *
 *   ./cpu_sim              跑全部
 *   ./cpu_sim --trace sum  只跑累加，并每拍打印 FETCH/DECODE/EXECUTE/WB
 */

static int fails;

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

static int run_named(const char *name, void (*load)(struct Cpu *), int trace)
{
    struct Cpu c;
    load(&c);
    c.trace = trace;
    if (trace) {
        printf("--- trace %s ---\n", name);
    }
    if (!cpu_run(&c, 10000)) {
        printf("FAIL  %s: did not HALT\n", name);
        ++fails;
        return 1;
    }
    if (strcmp(name, "sum") == 0) {
        return expect("sum 1..5 -> R0=15", c.r[0] == 15 && c.halted);
    }
    if (strcmp(name, "fib") == 0) {
        return expect("fib F7 -> R1=13", c.r[1] == 13 && c.halted);
    }
    if (strcmp(name, "memcpy") == 0) {
        int ok = c.dmem[0x20] == 0xAA && c.dmem[0x21] == 0xBB
              && c.dmem[0x22] == 0xCC && c.dmem[0x23] == 0xDD
              && c.dmem[0x10] == 0xAA;
        return expect("memcpy 4B 0x10 -> 0x20", ok && c.halted);
    }
    return expect(name, 0);
}

int main(int argc, char **argv)
{
    int trace = 0;
    const char *only = NULL;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--trace") == 0) {
            trace = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("usage: cpu_sim [--trace] [sum|fib|memcpy]\n");
            return 0;
        } else {
            only = argv[i];
        }
    }

    if (!only || strcmp(only, "sum") == 0) {
        run_named("sum", load_sum, trace);
    }
    if (!only || strcmp(only, "fib") == 0) {
        run_named("fib", load_fib, trace);
    }
    if (!only || strcmp(only, "memcpy") == 0) {
        run_named("memcpy", load_memcpy, trace);
    }

    if (only && strcmp(only, "sum") && strcmp(only, "fib") && strcmp(only, "memcpy")) {
        printf("unknown program: %s\n", only);
        return 2;
    }

    printf("cpu self-test %s (%d failed)\n", fails ? "FAILED" : "OK", fails);
    return fails ? 1 : 0;
}
