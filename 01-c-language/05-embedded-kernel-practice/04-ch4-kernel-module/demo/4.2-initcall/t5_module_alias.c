/* T5: 模拟模块版 module_init —— alias 属性 + 符号查找
 * 对应 module.h 的:
 *   #define module_init(initfn)  \
 *       int init_module(void) __copy(initfn) __attribute__((alias(#initfn))); \
 *       ___ADDRESSABLE(init_module, __initdata);
 *
 * 模块加载时内核从 ELF 符号表找 "init_module" 符号，拿到它的地址，
 * 存到 mod->init，然后 do_one_initcall(mod->init) 调用。
 *
 * 这里用 dlsym 模拟"从符号表查找"的过程。
 */
#include <stdio.h>
#include <dlfcn.h>

/* 模块作者写的真实初始化函数 */
static int my_driver_init(void)
{
    printf("  my_driver_init executed\n");
    return 0;
}

/* module_init(my_driver_init) 展开后的样子（简化） */
int init_module(void) __attribute__((alias("my_driver_init")));

int main(void)
{
    printf("T5: module_init alias simulation\n");

    /* 模拟内核 load_module 从 ELF 符号表找 init_module */
    /* 用 dlopen(NULL, RTLD_NOW) 拿到主程序自身的符号表 */
    void *handle = dlopen(NULL, RTLD_NOW);
    if (!handle) {
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
        return 1;
    }

    /* 模拟 find_symbol("init_module") */
    typedef int (*initcall_t)(void);
    initcall_t fn = (initcall_t)dlsym(handle, "init_module");
    const char *err = dlerror();
    if (err) {
        fprintf(stderr, "dlsym(init_module) failed: %s\n", err);
        dlclose(handle);
        return 1;
    }

    printf("  init_module symbol found at %p\n", (void*)fn);
    printf("  my_driver_init address   = %p\n", (void*)my_driver_init);
    printf("  same? %s\n", (fn == my_driver_init) ? "YES (alias works)" : "NO");

    /* 模拟 do_init_module: mod->init = init_module; do_one_initcall(mod->init) */
    printf("  calling mod->init (= init_module) ...\n");
    int ret = fn();
    printf("  ret=%d\n", ret);

    dlclose(handle);
    return 0;
}
