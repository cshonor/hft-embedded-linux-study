/* ex13_3_fflush_fsync.c — Ch13 习题 13-3：`fflush(fp); fsync(fileno(fp));` 到底做了什么
 *
 * 原书习题 13-3（p.250）：
 *   「下面两条语句的作用是什么？  fflush(fp);  fsync(fileno(fp)); 」
 *
 * 这题的答案不难背（一个刷用户态、一个刷内核态），但**两个顺序陷阱**很值得量：
 *   ① 只调 `fsync(fileno(fp))` 而**不先** `fflush(fp)` ——
 *      fsync 会返回 0（成功），可文件里**一个字节都没有**：
 *      因为写入还停在 stdio 缓冲里，fsync 根本不知道有这回事；
 *   ② 只调 `fflush(fp)` ——
 *      另一个 fd 已经能读到全部内容（说明进了页缓存），但**没有任何东西保证它落盘**。
 * 判据用三个能真量出来的量：
 *   - 「另一个 fd 看到多少字节」→ 数据有没有离开**本进程的用户态**；
 *   - `fsync` 的返回值 → 内核有没有把这批页交给设备；
 *   - `/proc/vmstat` 的 nr_dirty → 整机层面还有多少脏页（辅助，会被别人干扰）。
 *
 * 编译： gcc -O0 -Wall -Wextra -o ex13_3_fflush_fsync ex13_3_fflush_fsync.c
 * 取材： TLPI 习题 13-3；§13.2（fflush 的作用范围）、§13.3（fsync 的作用范围）
 *       man-pages 6.19 fflush(3)（"do not ... flush to disk"）、fsync(2)、fileno(3)
 *       Linux v6.6 fs/sync.c:205-226（do_fsync：只看 fd，不看打开模式）
 *       C99 7.19.5.3（fflush 只在输出流上定义良好）
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define F1 "/app/ex13_3.txt"
#define PAYLOAD "0123456789-ABCDEFGHIJ"        /* 21 字节 */
static const size_t PLEN = 21;

static long vmstat_val(const char *key)
{
    FILE *fp = fopen("/proc/vmstat", "r");
    char line[256];
    long v = -1;

    if (fp == NULL)
        return -1;
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strncmp(line, key, strlen(key)) == 0) {
            if (sscanf(line + strlen(key), " %ld", &v) != 1)
                v = -2;
            break;
        }
    }
    fclose(fp);
    return v;
}

/* 另一个 fd 能看到多少字节 */
static long other_fd_size(void)
{
    struct stat st;
    int fd = open(F1, O_RDONLY);
    long sz;

    if (fd == -1)
        return -1;
    sz = (fstat(fd, &st) == -1) ? -1 : (long) st.st_size;
    close(fd);
    return sz;
}

int main(void)
{
    printf("== 0. 先把两条语句分开 ≡ 先把两层分开 ==\n");
    printf("  `fflush(fp)`          ：把 **stdio 用户态缓冲** 交给内核（→ 页缓存）\n");
    printf("  `fsync(fileno(fp))`   ：把 **内核页缓存里属于这个文件的部分** 交给设备\n");
    printf("  载荷 \"%s\"（%zu 字节）\n\n", PAYLOAD, PLEN);

    printf("== ① 陷阱：只调 fsync、不先 fflush ==\n");
    {
        FILE *fp;
        int fd;

        unlink(F1);
        fp = fopen(F1, "w");
        if (fp == NULL) {
            printf("  fopen 失败 errno=%d(%s)\n", errno, strerror(errno));
            return EXIT_FAILURE;
        }
        fd = fileno(fp);
        fprintf(fp, "%s", PAYLOAD);
        printf("  已 fprintf %zu 字节（还在 stdio 缓冲里）\n", PLEN);
        printf("  另一个 fd 看到的文件大小 = %ld\n", other_fd_size());

        errno = 0;
        int r = fsync(fd);                 /* ← 故意不 fflush */
        printf("  fsync(fileno(fp)) = %d errno=%d(%s)  ← 内核说「成功了」\n",
               r, errno, strerror(errno));
        printf("  但另一个 fd 看到的文件大小仍然是 = %ld\n", other_fd_size());
        printf("  → **fsync 返回 0 并不代表你的数据安全了**：它只管它看得见的那一层，\n");
        printf("     而你的字节还在 stdio 缓冲里。这是本节最值得记住的一条。\n");
        printf("     nr_dirty=%ld（16 字节远小于一页，看不出变化，仅作记录）\n",
               vmstat_val("nr_dirty"));

        printf("\n  补上 fflush 再看：\n");
        errno = 0;
        int f = fflush(fp);
        printf("  fflush(fp) = %d errno=%d(%s)\n", f, errno, strerror(errno));
        printf("  另一个 fd 看到的文件大小 = %ld  ← 现在才可见\n", other_fd_size());
        errno = 0;
        r = fsync(fd);
        printf("  fsync(fileno(fp)) = %d errno=%d(%s)\n", r, errno, strerror(errno));
        fclose(fp);
    }

    printf("\n== ② 陷阱：只 fflush、不 fsync ==\n");
    {
        FILE *fp;
        int fd;

        unlink(F1);
        fp = fopen(F1, "w");
        if (fp == NULL) {
            printf("  fopen 失败 errno=%d(%s)\n", errno, strerror(errno));
            return EXIT_FAILURE;
        }
        fd = fileno(fp);
        fprintf(fp, "%s", PAYLOAD);
        fflush(fp);
        printf("  fileno(fp) = %d\n", fd);
        printf("  fflush 之后，另一个 fd 看到 %ld 字节 —— 数据已经离开本进程\n", other_fd_size());
        printf("  可这是**页缓存**；此刻拔电源，字节就没了。fflush **没有任何**持久化语义。\n");
        printf("  → 想让「外部可见」升级成「已落盘」，只有 fsync/fdatasync/sync/O_SYNC 这几条路。\n");
        fclose(fp);
    }

    printf("\n== ③ 两条语句连用：作用域是**串联**的，不是二选一 ==\n");
    {
        FILE *fp = fopen(F1, "w");
        int fd;
        int r1, r2;
        long before, after;

        if (fp == NULL) {
            printf("  fopen 失败 errno=%d(%s)\n", errno, strerror(errno));
            return EXIT_FAILURE;
        }
        fd = fileno(fp);
        before = vmstat_val("nr_dirty");
        fprintf(fp, "%s", PAYLOAD);
        printf("  状态表（✓ = 该层已有数据；? = 无法从用户态直接证实）\n");
        printf("    %-28s %-14s %-14s %s\n", "时刻", "stdio 用户缓冲", "内核页缓存", "磁盘");
        printf("    %-28s %-14s %-14s %s\n", "fprintf 之后", "✓", "—", "—");
        errno = 0;
        r1 = fflush(fp);
        printf("    %-28s %-14s %-14s %s\n", "fflush 之后", "—", "✓", "—");
        errno = 0;
        r2 = fsync(fd);
        printf("    %-28s %-14s %-14s %s\n", "fsync 之后", "—", "✓", "✓");
        after = vmstat_val("nr_dirty");
        printf("    fflush 返回 %d，fsync 返回 %d；nr_dirty %ld -> %ld\n", r1, r2, before, after);
        fclose(fp);
        printf("  → 这两条语句不是「两个可选方案」，而是**一次接力的两棒**：\n");
        printf("     少任何一棒，你要么丢数据，要么没落盘。\n");
    }

    printf("\n== ④ 工程上的等价写法与选择 ==\n");
    printf("  %-34s %s\n", "写法", "语义");
    printf("  %-34s %s\n", "fflush(fp)", "只推一层（用户态 → 页缓存）");
    printf("  %-34s %s\n", "fsync(fileno(fp))", "只推一层（页缓存 → 设备），**看不见** stdio 缓冲");
    printf("  %-34s %s\n", "fflush(fp); fsync(fileno(fp))", "两层都推 —— 本题的正确答案");
    printf("  %-34s %s\n", "setvbuf(fp,..,_IONBF,0) + write", "干脆不要用户态缓冲，只留一层");
    printf("  %-34s %s\n", "fdatasync(fileno(fp))", "同上，但允许不写纯时间戳类元数据");
    printf("  ⚠️ 常见误写：`fsync(fp)` —— `fp` 是 `FILE*`，**编译不过**（会当 int 警告）。\n");
    printf("     必须 `fsync(fileno(fp))`。`-Wall` 会提醒你类型不匹配，别忽略。\n");
    printf("  ⚠️ 另一个误写：反复 `fflush(fp)` —— 第二次是空操作，不会更安全。\n");
    printf("     真正的成本在 `fsync`，所以「多刷几次保险」只会白烧 I/O 预算。\n");

    unlink(F1);
    return EXIT_SUCCESS;
}
