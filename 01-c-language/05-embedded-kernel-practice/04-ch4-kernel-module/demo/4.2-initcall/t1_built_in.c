/* T1: 模拟内建版 module_init —— 用 __attribute__((section)) 把函数指针放进 .initcall6.init 段
 * 这就是内核 __define_initcall(fn, 6) 展开后的样子（简化版）。
 * 对比 init.h:
 *   static initcall_t __name __attribute__((used)) __attribute__((__section__(".initcall6.init"))) = fn;
 */
#include <stdio.h>

typedef int (*initcall_t)(void);

/* 模拟 module_init(fn) 内建版展开 */
#define SIM_INITCALL(fn) \
    static initcall_t __attribute__((used)) __attribute__((section(".initcall6.init"))) \
        __sim_initcall_##fn = fn

static int early_console(void)  { printf("  [0] early console\n"); return 0; }
static int core_driver(void)    { printf("  [1] core driver\n");   return 0; }
static int device_probe(void)   { printf("  [2] device probe\n");  return 0; }
static int late_policy(void)    { printf("  [3] late policy\n");   return 0; }

SIM_INITCALL(early_console);
SIM_INITCALL(core_driver);
SIM_INITCALL(device_probe);
SIM_INITCALL(late_policy);

int main(void)
{
    printf("T1: initcall pointers in .initcall6.init\n");
    printf("  early_console  fn=%p  slot=%p\n",
           (void*)early_console, (void*)&__sim_initcall_early_console);
    printf("  core_driver    fn=%p  slot=%p\n",
           (void*)core_driver,   (void*)&__sim_initcall_core_driver);
    printf("  device_probe   fn=%p  slot=%p\n",
           (void*)device_probe,  (void*)&__sim_initcall_device_probe);
    printf("  late_policy    fn=%p  slot=%p\n",
           (void*)late_policy,   (void*)&__sim_initcall_late_policy);
    return 0;
}
