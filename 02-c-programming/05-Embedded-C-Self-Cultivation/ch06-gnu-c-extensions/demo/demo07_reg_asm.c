/*
 * 寄存器约束与内嵌汇编（x86_64 / AArch64 通用写法）
 * ARM 裸机可改为 register int x asm("r0");
 */
#include <stdio.h>

static inline unsigned long read_hw_counter(void)
{
    unsigned long val;
#if defined(__aarch64__)
    asm volatile("mrs %0, cntvct_el0" : "=r"(val));
#elif defined(__x86_64__)
    asm volatile("rdtsc" : "=a"(val) : : "rdx");
#else
    val = 0;
#endif
    return val;
}

int main(void)
{
    unsigned long t0 = read_hw_counter();
    unsigned long t1 = read_hw_counter();

    printf("counter t0=%lu t1=%lu delta=%ld\n",
           t0, t1, (long)(t1 - t0));
    puts("ARM cross: register int val asm(\"r0\"); + volatile asm");
    return 0;
}
