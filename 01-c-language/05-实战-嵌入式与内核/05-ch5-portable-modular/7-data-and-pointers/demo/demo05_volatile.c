/*
 * 对比 volatile 与非 volatile 全局标志的汇编差异
 * objdump -dS demo05_volatile | less
 */
#include <stdio.h>

static volatile int irq_flag_volatile;
static int irq_flag_normal;

static void set_flags(void)
{
    irq_flag_volatile = 1;
    irq_flag_normal = 1;
}

static int poll_volatile(void)
{
    int n = 0;
    while (!irq_flag_volatile)
        n++;
    return n;
}

static int poll_normal(void)
{
    int n = 0;
    while (!irq_flag_normal)
        n++;
    return n;
}

int main(void)
{
    set_flags();
    printf("poll_volatile spins=%d poll_normal spins=%d\n",
           poll_volatile(), poll_normal());
    puts("Compare: objdump -d demo05_volatile | grep -A5 poll_");
    return 0;
}
