/* Ch7 §7.1 — program break 的查询与调整（brk / sbrk）
 * 编译: gcc -O2 -Wall -Wextra -o c7_1_program_break c7_1_program_break.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define PAGE 4096

/* ── A. 安全用法：只在「当前 break 之上」要内存，并只归还到自己的出发点 ── */
static void safe_use(void)
{
    /* 先输出一次，让 stdio 的缓冲区落到堆上。
     * 若把首次输出留到后面，归还 break 时会把 glibc 尚未分配的堆一并砍掉。 */
    printf("A. 安全用法\n");

    char *base = (char *) sbrk(0);      /* 此刻的 break 已包含 stdio 缓冲 */
    printf("   起始 break            = %p\n", (void *) base);

    /* sbrk(+n)：相对移动，成功时返回「调整前」的断点 */
    char *old = (char *) sbrk(16 * PAGE);
    if (old == (char *) -1) { perror("sbrk"); return; }
    printf("   sbrk(+%ld) 返回旧断点 = %p\n", 16L * PAGE, (void *) old);
    printf("   抬高后 break          = %p\n", sbrk(0));
    printf("   返回值 == 旧断点吗    = %s\n", old == base ? "是" : "否");

    /* 新区域可以随意读写：虚拟地址已到手，物理页还没分配 */
    old[0] = 'A';
    old[16 * PAGE - 1] = 'Z';
    printf("   首尾各写一字节        = %c ... %c\n", old[0], old[16 * PAGE - 1]);

    /* 归还：只回到 base（自己的出发点），一字节都不多降 */
    if (brk(base) != 0) { perror("brk"); return; }
    printf("   brk(起始) 后 break    = %p  ← 回到出发点\n\n", sbrk(0));
}

/* ── B. 危险用法：把 break 降到程序启动时的位置，会砍掉 glibc 自己的堆 ── */
static char *g_earliest;        /* main 第一行读到的 break：那时 stdio 缓冲还没分配 */

static void reckless_use(void)
{
    printf("B. 危险用法：把 break 降回「还没用到 stdio」时的位置\n");
    fflush(stdout);
    brk(g_earliest);            /* 连 glibc 的 stdio 缓冲区一起还给内核 */
    printf("   这一行若能看到，说明缓冲区恰好没被砍到\n");
}

int main(void)
{
    g_earliest = (char *) sbrk(0);

    safe_use();

    fflush(stdout);
    pid_t pid = fork();
    if (pid == 0) { reckless_use(); return 0; }

    int st = 0;
    waitpid(pid, &st, 0);
    if (WIFSIGNALED(st))
        printf("B. 子进程被信号 %d 杀死。用户态的 stdio 缓冲区也住在堆上，\n"
               "   把 break 降过头 = 把 glibc 自己的数据也还给了内核。\n"
               "   这正是「业务代码禁止直接 brk/sbrk」最硬的理由。\n",
               WTERMSIG(st));
    else
        printf("B. 子进程 exit = %d（这次没崩）\n", WEXITSTATUS(st));
    return 0;
}
