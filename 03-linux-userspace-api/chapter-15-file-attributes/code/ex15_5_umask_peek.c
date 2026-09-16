/* ex15_5_umask_peek.c — 习题 15-5：怎么「只看一眼」当前 umask 而不改它？
 *
 * 题面：umask() 总是「设置并返回旧值」，没有只读形态。怎么在不留下
 * 副作用的前提下拿到当前 umask？
 *
 * 答案（三选一，本程序全部演示）：
 *   ① 经典答案：umask(0) 取到旧值，**立刻**恢复——umask(SAVED)。
 *      缺陷：不是原子的。单线程程序安全；**多线程程序有竞态窗口**，
 *      窗口内其它线程创建的文件会拿到 0000 的 mode——TLPI §15.4.6
 *      明确点名这个 race；
 *   ② fork 一个子进程，在子进程里 umask(0) 读出后把值写回父进程
 *      （管道/exit code），父进程 umask 从未被动过——无竞态但重；
 *   ③ 读 /proc/self/status 的 Umask 字段（Linux 专有，只读、零副作用，
 *      macOS 没有这个文件，本机退化为 ①+②）。
 *
 * 编译： cc -Wall -Wextra -o ex15_5_umask_peek ex15_5_umask_peek.c
 * 取材： TLPI §15.4.6 习题 15-5；man-pages proc(5)（Umask 字段）
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#if !defined(__APPLE__)
/* ③ Linux 专有：/proc/self/status 的 Umask 行（内核 4.7+ 才有） */
static int peekUmaskProc(mode_t *out)
{
    FILE *fp = fopen("/proc/self/status", "r");
    if (!fp)
        return -1;
    char line[256];
    int found = 0;
    while (fgets(line, sizeof line, fp)) {
        if (strncmp(line, "Umask:", 6) == 0) {
            unsigned v;
            if (sscanf(line + 6, "%o", &v) == 1) {
                *out = (mode_t) v;
                found = 1;
            }
            break;
        }
    }
    fclose(fp);
    return found ? 0 : -1;
}
#endif

int main(void)
{
    /* 先故意设一个能认出来的值 */
    mode_t saved = umask(0027);
    printf("预置 umask = 0027（程序入口真实的旧值是 %04lo）\n\n",
           (unsigned long) saved);

    /* ---------- ① umask(0) + 恢复：单线程安全，多线程有竞态 ---------- */
    printf("== ① 经典读法：umask(0) → 立刻恢复 ==\n");
    mode_t cur = umask(0);
    umask(cur);                         /* 立刻放回去 */
    printf("  读到 umask = %04lo，已恢复（多线程时这是 0%03lo 的裸奔窗口）\n",
           (unsigned long) cur, (unsigned long) cur);
    printf("  ⚠️ 窗口内其它线程 open(mode=0666) 会真的拿到 0666 文件。\n");
    printf("     §15.4.6 的结论：umask 是进程属性且接口本身不原子，\n");
    printf("     多线程程序**不要**用 ①，也少用「临时改 umask」的把戏。\n\n");

    /* ---------- ② 子进程探测：父进程 umask 全程未动 ---------- */
    printf("== ② 子进程探测：竞态归零 ==\n");
    int pfd[2];
    if (pipe(pfd) == -1) { perror("pipe"); return EXIT_FAILURE; }
    pid_t pid = fork();
    if (pid == -1) { perror("fork"); return EXIT_FAILURE; }
    if (pid == 0) {                     /* 子进程：随便折腾 umask */
        mode_t u = umask(0);
        write(pfd[1], &u, sizeof u);
        _exit(0);
    }
    close(pfd[1]);
    mode_t child = 0;
    if (read(pfd[0], &child, sizeof child) != sizeof child) {
        perror("read"); return EXIT_FAILURE;
    }
    close(pfd[0]);
    int status;
    waitpid(pid, &status, 0);
    printf("  子进程读到 = %04lo，父进程 umask 仍是 %04lo（未被动过）\n",
           (unsigned long) child, (unsigned long) umask(umask(child) ));
    /* 上面那行是花哨写法：umask(child) 先读出当前值再设回 child，等价于不改 */

#if !defined(__APPLE__)
    /* ---------- ③ /proc/self/status：Linux 最省事 ---------- */
    printf("\n== ③ /proc/self/status 的 Umask 字段（Linux 专有）==\n");
    mode_t u3;
    if (peekUmaskProc(&u3) == 0) {
        printf("  Umask = %04lo（只读、零副作用、无竞态——Linux 4.7+ 首选）\n",
               (unsigned long) u3);
    } else {
        printf("  读取失败（内核 <4.7 或非 Linux）\n");
    }
#else
    printf("\n== ③ /proc/self/status：macOS 没有此文件，跳过（Linux 4.7+ 可用）==\n");
#endif

    return EXIT_SUCCESS;
}
