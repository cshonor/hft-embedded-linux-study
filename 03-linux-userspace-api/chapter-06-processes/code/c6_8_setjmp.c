/* c6_8_setjmp.c — 6.8 setjmp/longjmp：跨函数跳转 + 非 volatile 局部变量被回滚
 * 编译对比: gcc -O0 与 gcc -O2 各跑一次，看 auto_var 的差异
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <setjmp.h>
#include <stdlib.h>

static jmp_buf env;

/* 深层调用：出错就跳回 main 的 setjmp 点 */
static void level3(int fail)
{
    if (fail) {
        printf("    [level3] 出错，longjmp(env, 42)\n");
        longjmp(env, 42);
    }
    printf("    [level3] 正常返回\n");
}
static void level2(int fail) { level3(fail); }
static void level1(int fail) { level2(fail); }

int main(void)
{
    /* jmp_buf 里到底存了什么：x86-64 glibc = 8 个 long(保存 rbx/rbp/r12-r15/rsp/rip)
     * + int __mask_was_saved + 128B __sigset_t __saved_mask */
    printf("sizeof(jmp_buf) = %zu B，sizeof(sigjmp_buf) = %zu B\n",
           sizeof(jmp_buf), sizeof(sigjmp_buf));

    /* 在 setjmp 与 longjmp 之间改动的局部变量：
     * auto_var   没有 volatile -> 可能只活在寄存器里 -> longjmp 后被「回滚」
     * vol_var    有 volatile   -> 强制走内存 -> 值一定是最新的
     */
    volatile int vol_var = 100;
    int auto_var = 100;
    static int sta_var = 100;              /* static 在 .data，不受影响 */

    int r = setjmp(env);
    if (r == 0) {
        printf("--- 第一次返回 0：正常路径 ---\n");
        level1(0);                          /* 正常 */
        printf("  改三个变量：100 -> 200\n");
        vol_var = auto_var = sta_var = 200;
        level1(1);                          /* 触发 longjmp */
        printf("  这行永远不执行\n");
    } else {
        printf("--- longjmp 回来，r = %d ---\n", r);
        printf("  volatile int vol_var = %d   (200 正确)\n", vol_var);
        printf("  int          auto_var= %d   (%s)\n", auto_var,
               auto_var == 200 ? "200 正确" : "被回滚成 100 ！");
        printf("  static int   sta_var = %d   (200 正确)\n", sta_var);
    }
    printf("集齐 setjmp：返回 0 表示直接调用，val==0 会被强制改成 1\n");

    /* 补一个 val==0 的演示 */
    if (setjmp(env) == 0) longjmp(env, 0);
    else printf("  longjmp(env, 0) 实际返回 1（0 被刻意避开）\n");
    return 0;
}
