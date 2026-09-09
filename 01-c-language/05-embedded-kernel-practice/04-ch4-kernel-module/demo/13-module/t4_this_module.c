/*
 * T4: __this_module —— 指定初始化 + alias 的闭环
 *
 * modpost 生成的 .mod.c 模板（scripts/mod/modpost.c:1930）：
 *     __visible struct module __this_module
 *     __section(".gnu.linkonce.this_module") = {
 *         .name = KBUILD_MODNAME,
 *         .init = init_module,          ← module_init(xxx_init) 生成的 alias
 *         .exit = cleanup_module,       ← module_exit(xxx_exit) 生成的 alias
 *         .arch = MODULE_ARCH_INIT,
 *     };
 *
 * 这一段把 CH1/CH2 三件套全用上了：
 *   (1) __section(...)           ← CH2 6.6：把实例放进专属段
 *   (2) .init = init_module       ← CH1 6.2：指定初始化
 *   (3) init_module 是 xxx_init 的 alias ← CH2 6.9：__attribute__((alias))
 *
 * 所以 mod->init 不是运行时“查找”出来的 —— 它在编译期就被指定初始化
 * 填进了嵌入的 struct module 实例。内核 do_init_module 直接 mod->init()。
 *
 * 注意：__attribute__((alias(...))) 要求目标在同一个翻译单元里已定义。
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* 用户写的 init / exit（真实驱动里就是 hello_init / hello_exit） */
static int my_driver_init(void)
{
    printf("[my_driver_init] 模块初始化\n");
    return 0;
}
static void my_driver_exit(void)
{
    printf("[my_driver_exit] 模块卸载\n");
}

/*
 * module_init(x) 展开（模块版，省略 __copy / ___ADDRESSABLE）：
 *   static inline initcall_t __inittest(void) { return x; }   // 编译期类型检查
 *   int init_module(void) __attribute__((alias(#x)));
 *
 * alias 让 init_module 和 my_driver_init 指向同一地址，但名字不同。
 * 这样内核只需认标准名 init_module，不必知道用户起的名。
 */
typedef int  (*initcall_t)(void);
typedef void (*exitcall_t)(void);

static inline initcall_t __inittest(void)  { return my_driver_init; }
int init_module(void)    __attribute__((alias("my_driver_init")));

static inline exitcall_t __exittest(void)  { return my_driver_exit; }
void cleanup_module(void) __attribute__((alias("my_driver_exit")));

/* 极简 struct module（只留本 demo 关心的字段） */
struct mini_module {
    const char   *name;
    initcall_t    init;
    exitcall_t    exit;
};

/* === 对应 .mod.c 模板：指定初始化把 alias 填进专属段 ===
 * 段名用 "this_module"（无点，方便演示）；内核真名是 .gnu.linkonce.this_module
 */
__attribute__((used, section("this_module")))
struct mini_module __this_module = {
    .name = "my_driver",
    .init = init_module,        /* ← alias，== my_driver_init */
    .exit = cleanup_module,    /* ← alias，== my_driver_exit */
};

int main(void)
{
    printf("=== T4: __this_module 指定初始化 + alias ===\n\n");

    printf("--- 地址对照（alias 让两个名指向同一处） ---\n");
    printf("  my_driver_init  @ %p\n", (void*)(uintptr_t)my_driver_init);
    printf("  init_module    @ %p  (alias) -> %s\n",
           (void*)(uintptr_t)init_module,
           (void*)(uintptr_t)init_module == (void*)(uintptr_t)my_driver_init
               ? "与 my_driver_init 相同" : "不同（错！）");
    printf("  my_driver_exit @ %p\n", (void*)(uintptr_t)my_driver_exit);
    printf("  cleanup_module @ %p  (alias) -> %s\n",
           (void*)(uintptr_t)cleanup_module,
           (void*)(uintptr_t)cleanup_module == (void*)(uintptr_t)my_driver_exit
               ? "与 my_driver_exit 相同" : "不同（错！）");

    printf("\n--- __this_module 内容（编译期指定初始化的结果） ---\n");
    printf("  name = %s\n", __this_module.name);
    printf("  init = %p  exit = %p\n",
           (void*)(uintptr_t)__this_module.init,
           (void*)(uintptr_t)__this_module.exit);

    printf("\n--- do_init_module 调 mod->init()（经 alias 回到用户函数） ---\n");
    int ret = __this_module.init();   /* 对应 do_one_initcall(mod->init) */
    printf("  init 返回 %d\n", ret);

    printf("\n--- rmmod 调 mod->exit() ---\n");
    __this_module.exit();

    /* 关键点：mod->init 不是运行时查表查出来的 */
    printf("\n--- 关键：init_module 符号表里的形态 ---\n");
    /* nm/init_module 是 STB_GLOBAL 的绝对符号，不是 undefined */
    printf("  init_module 是 alias 定义（STB_GLOBAL），不是未决符号\n");
    printf("  → 模块加载时不需要 resolve_symbol(\"init_module\")\n");
    printf("  → 它随 .gnu.linkonce.this_module 段一起被 memcpy 到最终地址\n");
    printf("  → 重定位只修正 init_module 的绝对地址（PREL32/R_RELATIVE）\n");
    return 0;
}
