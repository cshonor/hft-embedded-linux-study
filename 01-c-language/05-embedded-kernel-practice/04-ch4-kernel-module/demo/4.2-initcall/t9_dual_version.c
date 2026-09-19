/* T9: module_init 双版本对比 —— 同一个宏，MODULE 定义与否完全不同
 * 对应 module.h 的 #ifdef MODULE ... #else ... #endif。
 * 用条件编译同时测试两个版本，对比生成的代码差异。
 */
#include <stdio.h>

/* === 模块版 (MODULE defined) ===
 * module.h:
 *   #define module_init(initfn) \
 *       int init_module(void) __copy(initfn) __attribute__((alias(#initfn))); \
 *       ___ADDRESSABLE(init_module, __initdata);
 * 效果：init_module 是 initfn 的别名，内核 dlsym 找 init_module。
 */
static int my_mod_init(void) { printf("  module init\n"); return 0; }
extern int init_module(void) __attribute__((alias("my_mod_init")));

/* === 内建版 (MODULE not defined) ===
 * init.h:
 *   #define module_init(x)  __initcall(x);  // → device_initcall → __define_initcall(fn, 6)
 *   → static initcall_t __name __attribute__((used)) __attribute__((section(".initcall6.init"))) = fn;
 * 效果：函数指针放进 .initcall6.init 段，启动循环找出来执行。
 */
typedef int (*initcall_t)(void);
static int my_builtin_init(void) { printf("  builtin init\n"); return 0; }
static initcall_t __attribute__((used)) __attribute__((section(".initcall6.init")))
    __builtin_initcall = my_builtin_init;

extern initcall_t __initcall_start[];
extern initcall_t __initcall_end[];

int main(void)
{
    printf("T9: module_init dual version\n");
    printf("  MODULE version:  init_module = %p (alias of my_mod_init = %p)\n",
           (void*)init_module, (void*)my_mod_init);
    printf("  builtin version: __builtin_initcall slot = %p, fn = %p\n",
           (void*)&__builtin_initcall, (void*)my_builtin_init);

    printf("\n  --- simulate insmod (module path) ---\n");
    /* 模块：内核 find_symbol("init_module") → mod->init = init_module → do_one_initcall */
    initcall_t mod_init = init_module;
    printf("  mod->init = %p\n", (void*)mod_init);
    mod_init();

    printf("\n  --- simulate boot (builtin path) ---\n");
    /* 内建：do_initcalls 遍历 __initcall_start..end */
    initcall_t *fn;
    for (fn = __initcall_start; fn < __initcall_end; fn++) {
        printf("  slot %p -> fn %p\n", (void*)fn, (void*)*fn);
        (*fn)();
    }
    return 0;
}
