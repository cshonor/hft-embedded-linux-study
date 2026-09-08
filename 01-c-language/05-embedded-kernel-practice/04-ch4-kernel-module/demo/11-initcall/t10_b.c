/* t10_b.c —— 驱动 B 的初始化函数 + initcall 注册 */
#include <stdio.h>
typedef int (*initcall_t)(void);
#define SIM_INIT(fn) \
    static initcall_t __attribute__((used)) __attribute__((section(".initcall6.init"))) \
        __sim_##fn = fn
int b_driver(void) { printf("  b_driver\n"); return 0; }
SIM_INIT(b_driver);
