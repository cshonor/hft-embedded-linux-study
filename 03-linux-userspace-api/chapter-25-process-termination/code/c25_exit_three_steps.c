/* c25_exit_three_steps.c

   ⚠️ 延伸 demo —— **原书没有这个程序**。

   原书 §25.1 用三行 bullet 说清了 exit() 依次做三件事：
     ① 按注册的【逆序】调用 exit handler（atexit / on_exit 注册的）
     ② flush stdio 流缓冲
     ③ 调用 _exit(status)

   但「① 在 ② 之前」这句话没法靠读文档建立直觉，得让顺序变成**可观察量**。
   做法是把两条通路混着用：
     · write(2)  → 直接进内核缓冲，**立刻**出现在 stdout
     · printf(3) → 先进用户态 FILE 缓冲，要等 flush 才出现
   于是「谁先出现」就等价于「内核缓冲 vs 用户态缓冲」，而 flush 的时刻
   会被精确定位到某个 write 之后。

   在 CE 上跑：stdout 不是终端（是 socket）⇒ glibc 对 stdout 用**全缓冲**，
   所以下面这个顺序是**确定**的。如果在真终端上跑，stdout 是行缓冲，
   printf 会被换行符逐个推出去，顺序就完全不同 —— 这本身就是 §25.4 的内容。

   编译: gcc -O0 -Wall -Wextra -o c25_exit_three_steps c25_exit_three_steps.c
   运行: ./c25_exit_three_steps
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* write() 的包装：只写固定字符串 */
static void
w(const char *s)
{
    write(STDOUT_FILENO, s, strlen(s));
}

/* 先跑的 handler（后注册）：只 write + printf，不 flush */
static void
handlerB(void)
{
    w("W2  来自 hB 的 write()      —— 立刻出来\n");
    printf("M2  来自 hB 的 printf()    —— 先进用户态缓冲\n");
}

/* 后跑的 handler（先注册）：中间插一次 fflush，把「此刻缓冲里有什么」钉死 */
static void
handlerA(void)
{
    w("W3  来自 hA 的 write()      —— 立刻出来\n");
    fflush(stdout);          /* ← 关键：把 M1、M2 推出去，证明它们一直躺在缓冲里 */
    printf("M3  来自 hA 的 printf()    —— 直到 exit() 第 ② 步才出来\n");
    w("W4  来自 hA 的 write()      —— 在它自己的 printf 之后！\n");
}

int
main(void)
{
    w("=== 图例：W* 走 write(2)（立刻可见）；M* 走 printf(3)（先进用户态缓冲）===\n");
    w("=== 观察点：M1/M2 什么时候出现，就等于 hA 那次 fflush 的位置 ===\n\n");

    printf("M1  来自 main 的 printf()  —— 先进用户态缓冲\n");
    w("W1  来自 main 的 write()    —— 立刻出来\n");

    if (atexit(handlerA) != 0 || atexit(handlerB) != 0) {
        w("atexit 注册失败\n");
        return EXIT_FAILURE;
    }
    w("\n--- 下面调用 exit(0)：handler 先跑（LIFO：hB 再 hA），最后才 flush ---\n");

    exit(0);   /* 不返回 */
}
