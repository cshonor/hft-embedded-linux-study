/* c25_exec_clears_handlers.c

   ⚠️ 延伸 demo —— **原书没有这个程序**。

   原书 §25.3 有一句不起眼但很重要的话：

     "A child process created via fork() inherits a copy of its parent's exit
      handler registrations. When a process performs an exec(), all exit handler
      registrations are removed. (This is necessarily so, since an exec() replaces
      the code of the exit handlers along with the rest of the existing program
      code.)"

   「fork 继承一份副本」在 Ch24 已经反复验证过；**「exec 全部清掉」**没验证过。
   本程序把两个事实放进同一次运行里对照：原实例注册一个 handler，然后
   exec 自己；新实例（同一份可执行文件）只注册自己的 handler。
   如果 exec 没有清掉注册，就会看到两行 handler 输出。

   ⚠️ 为什么用 /proc/self/exe 而不是 /bin/echo：
   本仓库的 CE 沙箱里 PATH 为空，/bin 与 /usr/bin 这两个目录整个不存在，
   唯一可用的绝对可执行路径就是 /proc/self/exe（Ch24 的 probe24 实测）。

   编译: gcc -O0 -Wall -Wextra -o c25_exec_clears_handlers c25_exec_clears_handlers.c
   运行: ./c25_exec_clears_handlers
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void
w(const char *s)
{
    write(STDOUT_FILENO, s, strlen(s));
}

/* exec 之前注册：应当**永远不跑** */
static void
oldHandler(void)
{
    w("  [!!] old-handler 跑了 —— 说明 exec 没有清掉注册（与原文不符）\n");
}

/* exec 之后由新实例注册：应当跑一次 */
static void
newHandler(void)
{
    w("  [ok] new-handler 跑了 —— 这是 exec 之后的新程序自己注册的\n");
}

int
main(int argc, char *argv[])
{
    if (argc > 1 && strcmp(argv[1], "after-exec") == 0) {
        /* ---------- exec 之后的那份程序 ---------- */
        w("[新实例] 我是 exec 之后的那份程序\n");
        w("[新实例] 检查上面有没有 old-handler 的输出 → 没有 ⇒ exec 清空了注册\n");
        if (atexit(newHandler) != 0) {
            w("[新实例] atexit 注册失败\n");
            return EXIT_FAILURE;
        }
        w("[新实例] 注册了自己的 handler，现在 exit(0)\n");
        exit(0);
        /* 不返回 */
    }

    /* ---------- exec 之前的那份程序 ---------- */
    w("[原实例] 先注册 old-handler\n");
    if (atexit(oldHandler) != 0) {
        w("[原实例] atexit 注册失败\n");
        return EXIT_FAILURE;
    }
    w("[原实例] 现在 exec(/proc/self/exe, \"after-exec\") —— 不 fork，PID 不变\n");
    fflush(NULL);                       /* exec 会保留 stdio 缓冲，先清空更干净 */
    execl("/proc/self/exe", "self", "after-exec", (char *) NULL);

    w("[原实例] exec 失败了 —— 这一行不该出现\n");
    return EXIT_FAILURE;
}
