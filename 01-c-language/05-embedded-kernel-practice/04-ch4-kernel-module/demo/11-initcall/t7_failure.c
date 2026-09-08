/* T7: initcall 失败处理 —— 返回非 0 的处理策略
 * 对应 do_one_initcall() 的 ret = fn(); 后的检查。
 * 内核不会在 initcall 失败时中止启动，但会 warn 并继续。
 * module 版本：失败则卸载模块（ret < 0 goto fail）。
 */
#include <stdio.h>

typedef int (*initcall_t)(void);

extern initcall_t __initcall_start[];
extern initcall_t __initcall_end[];

#define SIM_INIT(fn) \
    static initcall_t __attribute__((used)) __attribute__((section(".initcall6.init"))) \
        __sim_##fn = fn

static int ok_init(void)   { printf("  ok_init: success\n");        return 0; }
static int warn_init(void) { printf("  warn_init: ret=1\n");        return 1; }
static int err_init(void)  { printf("  err_init: ret=-ENODEV\n");   return -19; }
static int after_err(void) { printf("  after_err: still runs!\n"); return 0; }

SIM_INIT(ok_init);
SIM_INIT(warn_init);
SIM_INIT(err_init);
SIM_INIT(after_err);

/* 模拟 do_one_initcall：记录警告但继续 */
static int do_one_initcall(initcall_t fn)
{
    int ret = fn();
    if (ret < 0)
        printf("    -> ERROR %d (kernel: continues boot)\n", ret);
    else if (ret > 0)
        printf("    -> WARNING ret=%d (kernel: suspicious but continues)\n", ret);
    return ret;
}

static void do_initcalls(void)
{
    initcall_t *fn;
    int total = 0, failed = 0;
    for (fn = __initcall_start; fn < __initcall_end; fn++) {
        int ret = do_one_initcall(*fn);
        total++;
        if (ret) failed++;
    }
    printf("  total=%d failed=%d (boot continues regardless)\n", total, failed);
}

int main(void)
{
    printf("T7: initcall failure handling\n");
    do_initcalls();
    return 0;
}
