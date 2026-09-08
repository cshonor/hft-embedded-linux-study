/* t10_c.c —— 驱动 C 的初始化函数 + initcall 注册 */
#include <stdio.h>
typedef int (*initcall_t)(void);
#define SIM_INIT(fn) \
    static initcall_t __attribute__((used)) __attribute__((section(".initcall6.init"))) \
        __sim_##fn = fn
int c_driver(void) { printf("  c_driver\n"); return 0; }
SIM_INIT(c_driver);
