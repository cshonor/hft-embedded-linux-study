/*
 * c5_1_write_buffering.c —— strace 里最容易被看漏的一件事：write() 什么时候发生
 *
 * 用途：新手读 strace 最常见的困惑是「我 printf 了 5 次，为什么只有一条
 *       write(1, ...)」。答案是 stdio 缓冲：printf 只是往用户态缓冲区里放，
 *       真正调 write(2) 的时机由缓冲模式决定。这份程序把三种模式各跑一次，
 *       并且用一个「必然 SIGFPE」的结尾把差别**变成看得见的事实**：
 *
 *         缓冲输出在进程被信号杀死时会【丢】——因为缓冲区还没刷到内核。
 *
 * 用法：./c5_1_write_buffering <buf|line|nobuf>
 *
 * 编译：gcc -g -O0 -Wall -Wextra -o c5_1_write_buffering c5_1_write_buffering.c
 *
 * 实测（stdout 接管道 = 非终端）：
 *   nobuf → 三行都打出来，然后 SIGFPE（退出码 136）
 *   line  → 三行都打出来，然后 SIGFPE（退出码 136）
 *   buf   → 【一行都没有】，直接 SIGFPE（退出码 136）
 *
 * 对照：同样这份程序，stdout 接终端（tty）时 buf 也会正常打出三行 ——
 *       因为 tty 会让 stdout 变成**行缓冲**。这就是「同一个程序，重定向到文件
 *       就没输出了」的经典现象，也是 strace 里 write 次数变化的原因。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "buf";

    if (strcmp(mode, "nobuf") == 0)
        setvbuf(stdout, NULL, _IONBF, 0);        /* 无缓冲：每次 printf 立刻 write */
    else if (strcmp(mode, "line") == 0)
        setvbuf(stdout, NULL, _IOLBF, 0);        /* 行缓冲：见 '\n' 就 write */
    /* mode == "buf"：不动，用默认（非 tty 时是全缓冲 4096 字节） */

    /* 先说明当前环境，它决定了默认行为 —— strace 里也能看到 isatty 的痕迹 */
    printf("mode=%s  stdout_isatty=%d  (0=管道/文件→默认全缓冲, 1=终端→默认行缓冲)\n",
           mode, isatty(STDOUT_FILENO));
    printf("第 1 行：printf 已经把数据放进用户态缓冲区\n");
    printf("第 2 行：这时进程还没调用过 write(2)\n");

    /* ← 这里发 SIGFPE 杀进程：缓冲区里没刷出去的内容全部丢失 */
    volatile long volume = 0;
    volatile long turnover = 10700;
    printf("第 3 行：这行之后要除零了\n");
    printf("均价 = %ld\n", turnover / volume);

    printf("如果你看到这一行，说明除零没发生（不该发生）\n");
    return 0;
}
