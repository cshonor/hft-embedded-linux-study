/* T2: 完整模拟 initcall 机制 —— 链接脚本导出段起止符号 + 启动循环遍历执行
 * 这就是 init/main.c 的 do_initcalls() 用户态复刻：
 *   for (fn = __initcall_start; fn < __initcall_end; fn++) (*fn)();
 * 需要配套链接脚本 initcall.lds 导出 __initcall_start/__initcall_end。
 */
#include <stdio.h>

typedef int (*initcall_t)(void);

/* 外部符号：由链接脚本提供段起止地址 */
extern initcall_t __initcall_start[];
extern initcall_t __initcall_end[];

#define SIM_INITCALL(fn) \
    static initcall_t __attribute__((used)) __attribute__((section(".initcall6.init"))) \
        __sim_##fn = fn

static int console_init(void) { printf("  console up\n"); return 0; }
static int irq_init(void)     { printf("  irq subsystem\n"); return 0; }
static int net_init(void)     { printf("  network stack\n"); return 0; }
static int fs_init(void)      { printf("  filesystems\n"); return 0; }
static int drv_init(void)     { printf("  device drivers\n"); return 0; }

SIM_INITCALL(console_init);
SIM_INITCALL(irq_init);
SIM_INITCALL(net_init);
SIM_INITCALL(fs_init);
SIM_INITCALL(drv_init);

/* 模拟 do_initcalls() */
static void do_initcalls(void)
{
    initcall_t *fn;
    int count = 0;
    printf("do_initcalls: __initcall_start=%p __initcall_end=%p\n",
           (void*)__initcall_start, (void*)__initcall_end);
    for (fn = __initcall_start; fn < __initcall_end; fn++) {
        printf("  [%d] calling %p ... ", count++, (void*)*fn);
        int ret = (*fn)();
        printf("ret=%d\n", ret);
    }
    printf("total %d initcalls executed\n", count);
}

int main(void)
{
    printf("T2: do_initcalls simulation\n");
    do_initcalls();
    return 0;
}
