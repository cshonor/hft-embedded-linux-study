/* c15_2_timestamps.c — Ch15 §15.2：三个时间戳，哪些操作动哪一个
 *
 * TLPI §15.2 的核心表：「last access (atime) / last modification (mtime) /
 * last status change (ctime)」。本 demo 逐一实测四类操作：
 *   ① 写入内容 → mtime + ctime 动，atime 不动；
 *   ② 读内容   → 只有 atime 动（Linux 上 relatime 常常连 atime 都不动，
 *      本机 macOS 是「老派」行为，读一次动一次）；
 *   ③ chmod / chown 这类「改元数据」→ 只有 ctime 动；
 *   ④ rename 改的是目录项，文件自身的三个戳一个都不动（rename 改的是
 *      目录的 mtime/ctime，不是文件的）。
 * 另外钉住：**mtime 可以被手工设到未来/过去（15.2.1/15.2.2 的接口），ctime
 * 永远是内核盖的章，谁也改不了** —— 排查入侵/审计时只信 ctime。
 *
 * 编译： cc -Wall -Wextra -o c15_2_timestamps c15_2_timestamps.c
 * 取材： man-pages 6.19 stat(2)（三个时间戳的定义段）
 *       Linux v6.6 fs/inode.c（atime 更新策略 relatime）
 *       TLPI §15.2 表 15-1（哪些系统调用改变哪些时间戳）
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(__APPLE__)
#define AT(sb)  ((sb)->st_atimespec)
#define MT(sb)  ((sb)->st_mtimespec)
#define CT(sb)  ((sb)->st_ctimespec)
#else
#define AT(sb)  ((sb)->st_atim)
#define MT(sb)  ((sb)->st_mtim)
#define CT(sb)  ((sb)->st_ctim)
#endif

static void dump(const char *tag, const struct stat *sb)
{
    printf("  %-28s atime=%ld.%09ld  mtime=%ld.%09ld  ctime=%ld.%09ld\n",
           tag, (long) AT(sb).tv_sec, (long) AT(sb).tv_nsec,
           (long) MT(sb).tv_sec, (long) MT(sb).tv_nsec,
           (long) CT(sb).tv_sec, (long) CT(sb).tv_nsec);
}

static void read_file(const char *path)
{
    char buf[256];
    int fd = open(path, O_RDONLY);
    if (fd == -1) { perror("open(read)"); exit(EXIT_FAILURE); }
    while (read(fd, buf, sizeof buf) > 0)
        ;
    close(fd);
}

static void diff_and_note(const char *op, const struct stat *before,
                          const struct stat *after)
{
    int at = AT(before).tv_sec != AT(after).tv_sec ||
             AT(before).tv_nsec != AT(after).tv_nsec;
    int mt = MT(before).tv_sec != MT(after).tv_sec ||
             MT(before).tv_nsec != MT(after).tv_nsec;
    int ct = CT(before).tv_sec != CT(after).tv_sec ||
             CT(before).tv_nsec != CT(after).tv_nsec;
    printf("  → %s 动了: %s%s%s\n", op,
           at ? "atime " : "", mt ? "mtime " : "", ct ? "ctime" : "");
}

int main(void)
{
    const char *path = "/tmp/tlpi_c15_2.txt";
    struct stat before, after;

    unlink(path);
    int fd = open(path, O_RDWR | O_CREAT | O_EXCL, 0644);
    if (fd == -1) { perror("open"); return EXIT_FAILURE; }
    if (write(fd, "seed\n", 5) != 5) { perror("write"); return EXIT_FAILURE; }
    sleep(1);   /* 让时间戳分辨率能区分出变化 */

    printf("== ① 写内容：mtime + ctime ==\n");
    if (fstat(fd, &before) == -1) { perror("fstat"); return EXIT_FAILURE; }
    dump("写前", &before);
    sleep(1);
    if (write(fd, "more data\n", 10) != 10) { perror("write2"); return EXIT_FAILURE; }
    if (fstat(fd, &after) == -1) { perror("fstat"); return EXIT_FAILURE; }
    dump("写后", &after);
    diff_and_note("写内容", &before, &after);
    printf("  ⚠️ ctime 跟着 mtime 一起动：内容变了，「状态」必然也变了。\n\n");

    printf("== ② 读内容：只有 atime（本机行为实测）==\n");
    if (stat(path, &before) == -1) { perror("stat"); return EXIT_FAILURE; }
    dump("读前", &before);
    sleep(1);
    read_file(path);
    if (stat(path, &after) == -1) { perror("stat2"); return EXIT_FAILURE; }
    dump("读后", &after);
    diff_and_note("读内容", &before, &after);
    printf("  ⚠️ Linux 挂载带 relatime 时，atime 只在「mtime 更新 or 超过 24h」\n");
    printf("     才动——读多次可能一次都不动。依赖 atime 做「已读」判据不可靠。\n\n");

    printf("== ③ chmod：只动 ctime ==\n");
    if (stat(path, &before) == -1) { perror("stat3"); return EXIT_FAILURE; }
    dump("chmod 前", &before);
    sleep(1);
    if (chmod(path, 0600) == -1) { perror("chmod"); return EXIT_FAILURE; }
    if (stat(path, &after) == -1) { perror("stat4"); return EXIT_FAILURE; }
    dump("chmod 后", &after);
    diff_and_note("chmod", &before, &after);
    printf("  ⚠️ chmod 不碰内容 → atime/mtime 纹丝不动，ctime 独动。\n\n");

    printf("== ④ rename：Linux 不动文件戳；macOS 实测会动 ctime（跨平台差异）==\n");
    const char *path2 = "/tmp/tlpi_c15_2_renamed.txt";
    unlink(path2);
    if (stat(path, &before) == -1) { perror("stat5"); return EXIT_FAILURE; }
    dump("rename 前", &before);
    sleep(1);
    if (rename(path, path2) == -1) { perror("rename"); return EXIT_FAILURE; }
    if (stat(path2, &after) == -1) { perror("stat6"); return EXIT_FAILURE; }
    dump("rename 后", &after);
    diff_and_note("rename", &before, &after);
    printf("  ⚠️ 书上/Linux 的规则：rename 只改目录项（inode 不变），动的是\n");
    printf("     「目录」的时间戳，文件自身 atime/mtime/ctime 全不动；\n");
    printf("     但本机 macOS(BSD) 实测 ctime 动了 —— POSIX 把「移动后是否更新\n");
    printf("     文件自身 ctime」留给了实现，两边行为不同。跨平台审计代码\n");
    printf("     别拿「rename 后 ctime 必不变」当不变量；inode 编号不变才是硬事实。\n");

    close(fd);
    unlink(path2);
    return EXIT_SUCCESS;
}
