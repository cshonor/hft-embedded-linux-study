/* TLPI 第 2 章 §2.1 —— 库函数 vs 系统调用：三条写路径 + 缓冲时机
 *
 * 编译：gcc -O0 -Wall -Wextra c2_1_syscall.c -o c2_1
 * 运行：./c2_1
 *
 * 本节要钉死的事实：
 *   ① write() 是 glibc 对 sys_write 的薄封装，printf() 是「缓冲 + 多次 syscall」。
 *   ② strlen()/sizeof 是纯用户态，一次系统调用都不发。
 *   ③ 系统调用号是固定 ABI，写死在 arch/x86/entry/syscalls/syscall_64.tbl。
 *   ④ stdio 默认全缓冲（stdout 接管道/文件时），printf 的输出会「迟到」——
 *      与 write 混用时顺序会颠倒，这是真实可复现的现象。
 *
 * ⚠️ 注意 write() 的第三个参数是**字节数**，不是字符数。
 *    下面一律用 strlen() 算，硬编码长度会在多字节字符（中文）上截断成乱码。
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>

int main(void)
{
    /* ---------- ① 三条路径写同一句话 ---------- */
    printf("=== ① 三条路径写同一句话 ===\n");
    fflush(stdout);                     /* 先清干净，下面好观察顺序 */

    const char *m1 = "[write]    hello from write()\n";
    write(STDOUT_FILENO, m1, strlen(m1));          /* glibc 薄封装 → sys_write */

    const char *m2 = "[syscall]  hello from syscall(SYS_write)\n";
    syscall(SYS_write, STDOUT_FILENO, m2, strlen(m2));   /* 绕过 glibc 直接发号 */

    printf("[printf]   hello from printf()\n");
    printf("  三条都落在同一个 fd 1；前两条立即进内核，第三条先入用户态缓冲。\n");
    fflush(stdout);

    /* ---------- ② 纯用户态函数：一次 syscall 都不发 ---------- */
    printf("\n=== ② 纯用户态函数（不进内核）===\n");
    printf("  strlen(\"hello\")  = %zu   <- 运行期循环数到 '\\0'\n", strlen("hello"));
    printf("  sizeof(\"hello\")  = %zu   <- 编译期常量，连循环都不用\n", sizeof("hello"));
    printf("  strlen 和 sizeof 都可能有 0 次系统调用。\n");
    printf("  区分方法：man 2 = 系统调用，man 3 = 库函数\n");

    /* ---------- ③ 系统调用号是固定 ABI ---------- */
    printf("\n=== ③ 系统调用号（x86-64，固定 ABI）===\n");
    printf("  SYS_read     = %d\n", SYS_read);
    printf("  SYS_write    = %d\n", SYS_write);
    printf("  SYS_open     = %d\n", SYS_open);
    printf("  SYS_close    = %d\n", SYS_close);
    printf("  SYS_mmap     = %d\n", SYS_mmap);
    printf("  SYS_fork     = %d\n", SYS_fork);
    printf("  SYS_execve   = %d\n", SYS_execve);
    printf("  SYS_getpid   = %d\n", SYS_getpid);
    printf("  <- 号码写死在 arch/x86/entry/syscalls/syscall_64.tbl，\n");
    printf("     内核用 sys_call_table[nr] 查表分发（见本节正文）\n");

    /* ---------- ④ 缓冲时机：printf 会「迟到」 ---------- */
    printf("\n=== ④ 缓冲时机实验（本节关键）===\n");
    printf("  isatty(STDOUT_FILENO) = %d  (0 = stdout 接的是管道/文件 → 全缓冲)\n",
           isatty(STDOUT_FILENO));
    fflush(stdout);

    printf("  [A] 这行由 printf 写入 stdio 缓冲，此刻还没进内核\n");
    const char *mb = "  [B] 这行由 write 直接落到 fd 1\n";
    write(STDOUT_FILENO, mb, strlen(mb));
    printf("  [C] 又一行 printf\n");
    printf("  [D] 还是 printf\n");
    fflush(stdout);                     /* 手动把缓冲推下去 */

    printf("  -> 实际顺序是 B 跑到 A/C/D 前面：stdio 这一层缓冲在「骗」你。\n");
    printf("     排查别人的输出顺序怪事时，先看 isatty() 和 fflush()。\n");
    printf("  -> 反过来，printf 的缓冲也是性能手段：凑满一块再发一次 syscall。\n");

    printf("\n=== ⑤ 一个可以直接量出来的数字 ===\n");
    printf("  BUFSIZ            = %d  (stdio 默认缓冲大小)\n", BUFSIZ);
    printf("  一次 1000 字节的 printf 只发 1 次 write；\n");
    printf("  1000 次 1 字节的 write 要发 1000 次 syscall。\n");
    printf("  差别 = 999 次「用户态↔内核态」模式切换。\n");
    return 0;
}
