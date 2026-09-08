/* t10_a.c —— 驱动 A 的初始化函数 + initcall 注册 */
#include <stdio.h>
typedef int (*initcall_t)(void);
#define SIM_INIT(fn) \
    static initcall_t __attribute__((used)) __attribute__((section(".initcall6.init"))) \
        __sim_##fn = fn
int a_driver(void) { printf("  a_driver\n"); return 0; }
SIM_INIT(a_driver);
