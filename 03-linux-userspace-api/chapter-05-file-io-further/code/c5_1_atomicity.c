/* c5_1_atomicity.c — §5.1 原子性与竞态条件：两种经典"非原子"写法
 *
 * 原书用两条 shell 命令并发跑同一个程序来演示竞态：
 *     atomic_append f1 1000000 & atomic_append f1 1000000       → 正好 2 000 000 字节
 *     atomic_append f2 1000000 x & atomic_append f2 1000000 x   → 少一截（丢数据）
 * CE 的执行模型是「一个进程跑一次」，没法敲两条 shell 命令，所以这里用 fork()
 * 把两个写者放进同一进程树，复现同一个竞态。
 *
 * 演示三段：
 *   1) O_APPEND 与 lseek+write 的差别（内核是否帮你把「定位+写」做成一步）
 *   2) 检查文件是否存在、再创建 —— 两步之间被别人插队（Listing 5-1 的坑）
 *   3) 用 O_CREAT|O_EXCL 让「检查+创建」变成一个原子操作
 *
 * 编译: gcc -O0 -Wall -Wextra -o c5_1_atomicity c5_1_atomicity.c
 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#define PER_WRITER 200000       /* 每个写者写这么多字节；两个一起 400000 */

enum job { JOB_APPEND, JOB_LSEEK, JOB_CREATE_NOEXCL, JOB_CREATE_EXCL };

/* 子进程要做的事 */
static int do_job(enum job j, const char *path)
{
    if (j == JOB_CREATE_NOEXCL) {
        int fd = open(path, O_WRONLY);              /* 第一步：它存在吗？ */

        if (fd >= 0) {
            printf("  [PID %5ld] 文件已存在，我没创建它\n", (long) getpid());
            close(fd);
            return 0;
        }
        sleep(1);                                   /* 两步之间的窗口（原书用 sleep 放大） */
        fd = open(path, O_WRONLY | O_CREAT, 0644);  /* 第二步：那我来创建 */
        if (fd < 0)
            return 5;
        printf("  [PID %5ld] 不存在 → 我创建了它！\n", (long) getpid());
        close(fd);
        return 0;
    }

    if (j == JOB_CREATE_EXCL) {
        int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0644);

        if (fd < 0) {
            printf("  [PID %5ld] O_EXCL 失败 errno=%d (%s)：别人抢先创建了\n",
                   (long) getpid(), errno, strerror(errno));
            return 0;
        }
        printf("  [PID %5ld] O_EXCL 成功：确实是我创建的（唯一赢家）\n", (long) getpid());
        close(fd);
        return 0;
    }

    /* 一次一字节地写：JOB_APPEND 用 O_APPEND，JOB_LSEEK 用 lseek+write */
    {
        int flags = O_WRONLY | O_CREAT | (j == JOB_APPEND ? O_APPEND : 0);
        int fd = open(path, flags, 0644);

        if (fd < 0)
            return 2;
        for (int i = 0; i < PER_WRITER; i++) {
            if (j == JOB_LSEEK) {
                /* 非原子版本：先定位到末尾…… */
                if (lseek(fd, 0, SEEK_END) == (off_t) -1)
                    return 3;
            }
            /* ……再写。这两步之间别的进程可能也在往里写，于是互相覆盖 */
            if (write(fd, "x", 1) != 1) {
                close(fd);
                return 4;
            }
        }
        close(fd);
        return 0;
    }
}

static long long size_of(const char *p)
{
    struct stat st;

    return stat(p, &st) == 0 ? (long long) st.st_size : -1;
}

/* 两个子进程并发跑同一件事，父进程等它们都结束 */
static void run_twins(enum job j, const char *path)
{
    for (int i = 0; i < 2; i++) {
        if (fork() == 0) {
            int rc = do_job(j, path);

            /* 子进程继承的是块缓冲（stdout 被重定向时），_exit() 不刷缓冲，
               所以子进程自己的 printf 必须手动 flush，否则整段输出丢失 */
            fflush(NULL);
            if (rc != 0)
                fprintf(stderr, "child rc=%d\n", rc);
            _exit(0);
        }
    }
    while (wait(NULL) > 0)
        ;
}

int main(void)
{
    const char *fa = "/tmp/c5_append.bin";
    const char *fb = "/tmp/c5_lseek.bin";
    const char *fc = "/tmp/c5_race_noexcl";
    const char *fd = "/tmp/c5_race_excl";
    long long want = 2LL * PER_WRITER;

    unlink(fa); unlink(fb); unlink(fc); unlink(fd);

    printf("== 1. 两个写者并发写同一文件（各 %d 字节，一次一字节）==\n", PER_WRITER);
    fflush(stdout);

    printf("\n-- O_APPEND：内核把「移到末尾 + 写」做成不可分割的一步 --\n");
    fflush(stdout);
    run_twins(JOB_APPEND, fa);
    printf("  写完后 st_size = %lld（期望 %lld）\n", size_of(fa), want);

    printf("\n-- lseek+write：两步之间有窗口，两个进程会互相踩 --\n");
    fflush(stdout);
    run_twins(JOB_LSEEK, fb);
    printf("  写完后 st_size = %lld（期望 %lld）-> 丢 %lld 字节\n",
           size_of(fb), want, want - size_of(fb));

    printf("\n== 2. 先 open 检查、再 open 创建（没有 O_EXCL）==\n");
    fflush(stdout);
    run_twins(JOB_CREATE_NOEXCL, fc);
    printf("  两个进程都声称「我创建了它」——但它们只创建了一个文件\n");

    printf("\n== 3. 换成 O_CREAT|O_EXCL：检查与创建合成一个原子操作 ==\n");
    fflush(stdout);
    run_twins(JOB_CREATE_EXCL, fd);

    return 0;
}
