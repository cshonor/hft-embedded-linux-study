/* T6: 模拟 __init 段释放 —— 把初始化代码放进 .init.text，执行后 munmap 释放
 * 对应内核的 free_initmem()：把 .init.text / .init.data 段的页还给 buddy。
 * 内核用 __init 标记函数 → 放进 .init.text 段 → 启动后释放。
 *
 * 用户态用 mmap 复刻：把 init 函数的代码页映射一份，执行完 munmap。
 * 但更实际的做法是直接用 readelf 观察段的布局，并验证"执行后那段地址不可达"。
 */
#include <stdio.h>
#include <string.h>

typedef int (*initcall_t)(void);

extern initcall_t __initcall_start[];
extern initcall_t __initcall_end[];

#define SIM_INIT(fn) \
    static int fn(void) __attribute__((section(".init.text"), noinline)); \
    static initcall_t __attribute__((used)) __attribute__((section(".initcall6.init"))) \
        __sim_##fn = fn; \
    static int fn(void)

SIM_INIT(early_init)  { printf("  early init\n");  return 0; }
SIM_INIT(middle_init) { printf("  middle init\n"); return 0; }
SIM_INIT(late_init)   { printf("  late init\n");   return 0; }

/* 模拟 free_initmem：标记 init 段"已释放" */
static int init_freed = 0;

static void free_initmem(void)
{
    printf("  free_initmem: marking .init.text as freed\n");
    init_freed = 1;
}

static void do_initcalls(void)
{
    initcall_t *fn;
    for (fn = __initcall_start; fn < __initcall_end; fn++) {
        if (init_freed) {
            printf("  BUG: accessing freed init memory at %p!\n", (void*)*fn);
            continue;
        }
        (*fn)();
    }
}

int main(void)
{
    printf("T6: __init section lifecycle\n");
    printf("--- boot phase ---\n");
    do_initcalls();
    printf("--- post-boot: free_initmem ---\n");
    free_initmem();
    printf("--- calling init again (should warn) ---\n");
    do_initcalls();
    return 0;
}
