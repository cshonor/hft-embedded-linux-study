/* T4: KEEP() vs 无 KEEP —— gc-sections 会删未引用的 initcall 段吗？
 * 对应 vmlinux.lds.h 的 KEEP(*(.initcallN.init))。
 * 内核必须用 KEEP() 否则链接器会把"没有其他引用"的 initcall 段当死代码删掉。
 *
 * 关键坑：KEEP 保留的是段里的**指针**，不保留指针**指向的函数代码**。
 * 如果函数只被 initcall 段引用（没有其他引用），--gc-sections 会删函数代码，
 * 但段里的指针还在 → 调用就 segfault。
 * 内核用 ___ADDRESSABLE 宏确保符号被引用（不被 gc 删）。
 */
#include <stdio.h>

typedef int (*initcall_t)(void);

extern initcall_t __initcall_keep_start[];
extern initcall_t __initcall_keep_end[];

#define SIM_KEEP(fn) \
    static initcall_t __attribute__((used)) __attribute__((section(".initcall_keep"))) \
        __keep_##fn = fn

/* 注意：k1/k2/k3 没有标 used —— 只被 initcall 段引用。
 * --gc-sections 会删函数代码，但 KEEP 保留了段里的指针 → 调用时 segfault。
 * 这就是内核用 ___ADDRESSABLE 确保符号被引用的原因。
 */
static int k1(void) { printf("  k1\n"); return 0; }
static int k2(void) { printf("  k2\n"); return 0; }
static int k3(void) { printf("  k3\n"); return 0; }

SIM_KEEP(k1);
SIM_KEEP(k2);
SIM_KEEP(k3);

int main(void)
{
    printf("T4: KEEP() test\n");
    /* 如果链接脚本用 KEEP，段被保留；如果没用 KEEP 且 --gc-sections，段被删 */
    initcall_t *start = &__initcall_keep_start[0];
    initcall_t *end   = &__initcall_keep_end[0];
    printf("  keep_start=%p keep_end=%p count=%ld\n",
           (void*)start, (void*)end, end - start);
    if (end > start) {
        initcall_t *fn;
        for (fn = start; fn < end; fn++) {
            (*fn)();
        }
        printf("  KEEP preserved %ld entries\n", end - start);
    } else {
        printf("  gc-sections removed the section (need KEEP!)\n");
    }
    return 0;
}
