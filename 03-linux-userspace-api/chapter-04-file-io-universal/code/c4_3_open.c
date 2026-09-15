/* c4_3_open.c — open() 的 flags 与 mode：权限到底怎么算出来的
 *
 * 演示四件事：
 *   1) 最终权限 = mode & ~umask，用三组 umask 实测
 *   2) O_EXCL + O_CREAT = 原子排他创建（第二次必失败 EEXIST）
 *   3) O_TRUNC 清空 / O_APPEND 追加，同一个文件两种命运
 *   4) O_CLOEXEC 落在「fd 标志位」里，不在文件状态标志里
 *
 * 编译: gcc -O0 -Wall -o c4_3_open c4_3_open.c
 */
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

static void show(const char *tag, const char *path)
{
    struct stat st;
    if (stat(path, &st) < 0) { printf("  %s: stat 失败\n", tag); return; }
    printf("  %-28s 权限 = %04o (%s%s%s%s%s%s%s%s%s)  大小 = %lld\n",
           tag, st.st_mode & 07777,
           (st.st_mode & S_IRUSR) ? "r" : "-", (st.st_mode & S_IWUSR) ? "w" : "-",
           (st.st_mode & S_IXUSR) ? "x" : "-", (st.st_mode & S_IRGRP) ? "r" : "-",
           (st.st_mode & S_IWGRP) ? "w" : "-", (st.st_mode & S_IXGRP) ? "x" : "-",
           (st.st_mode & S_IROTH) ? "r" : "-", (st.st_mode & S_IWOTH) ? "w" : "-",
           (st.st_mode & S_IXOTH) ? "x" : "-",
           (long long) st.st_size);
}

int main(void)
{
    const mode_t MODE = 0666;               /* 请求的权限：rw-rw-rw- */
    const char *path = "/tmp/c4_open.txt";

    printf("== 1. 最终权限 = mode & ~umask  （本节核心）==\n");
    printf("  请求 mode = %04o (rw-rw-rw-)\n\n", MODE);

    mode_t masks[] = { 0000, 0022, 0077, 0027 };
    for (unsigned i = 0; i < sizeof masks / sizeof masks[0]; i++) {
        unlink(path);                        /* 先删掉，确保每次都是新建 */
        mode_t old = umask(masks[i]);
        int fd = open(path, O_WRONLY | O_CREAT, MODE);
        umask(old);                          /* 恢复，别影响后面 */
        close(fd);
        printf("  umask = %04o  ->  mode & ~umask = %04o & %04o = %04o\n",
               masks[i], (unsigned) MODE, (unsigned) (~masks[i] & 07777),
               (unsigned) (MODE & ~masks[i]));
        show("          实测", path);
    }
    printf("\n  结论：umask 只能「减」不能「加」。mode 里没有的位，umask 造不出来。\n");

    printf("\n== 2. O_EXCL + O_CREAT：原子的「不存在才创建」==\n");
    unlink(path);
    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0644);
    printf("  第一次 open(O_CREAT|O_EXCL) -> %d  （成功）\n", fd);
    close(fd);

    errno = 0;
    int fd2 = open(path, O_WRONLY | O_CREAT | O_EXCL, 0644);
    printf("  第二次 open(O_CREAT|O_EXCL) -> %d  errno=%d (%s)\n",
           fd2, errno, strerror(errno));
    printf("  → 两次 open 之间没有「先 stat 再 create」的窗口，这就是它防 TOCTOU 的意义。\n");

    printf("\n== 3. O_TRUNC 清空 vs O_APPEND 追加 ==\n");
    unlink(path);
    fd = open(path, O_WRONLY | O_CREAT, 0644);
    write(fd, "AAAA", 4);
    close(fd);
    show("写入 AAAA 后", path);

    fd = open(path, O_WRONLY | O_TRUNC);     /* 打开就截断成 0 */
    close(fd);
    show("O_TRUNC 打开一次后", path);

    fd = open(path, O_WRONLY | O_APPEND);
    write(fd, "BBBB", 4);
    close(fd);
    fd = open(path, O_WRONLY | O_APPEND);
    write(fd, "CCCC", 4);
    close(fd);
    show("O_APPEND 两次写 BBBB+CCCC", path);
    printf("  → O_TRUNC 只要「打开」就把数据清零，连 write 都不需要。\n");

    printf("\n== 4. O_CLOEXEC 存在哪：fd 标志 vs 文件状态标志 ==\n");
    int e1 = open("/dev/null", O_RDONLY | O_CLOEXEC);
    int e2 = open("/dev/null", O_RDONLY);
    int f1 = fcntl(e1, F_GETFD);             /* fd 标志：FD_CLOEXEC 在这 */
    int f2 = fcntl(e1, F_GETFL);             /* 文件状态标志：O_* 在这 */
    printf("  O_CLOEXEC 打开后  F_GETFD = 0x%x  (FD_CLOEXEC=0x%x → %s)\n",
           f1, FD_CLOEXEC, (f1 & FD_CLOEXEC) ? "已设" : "未设");
    printf("                    F_GETFL = 0x%x\n", f2);
    printf("      O_ACCMODE(0x%x) 取出的访问模式 = %d → %s\n",
           O_ACCMODE, f2 & O_ACCMODE,
           (f2 & O_ACCMODE) == O_RDONLY ? "只读" :
           (f2 & O_ACCMODE) == O_WRONLY ? "只写" : "读写");
    printf("      O_CLOEXEC 这一位在 F_GETFL 里吗？0x%x & 0x%x = 0x%x → %s\n",
           f2, O_CLOEXEC, f2 & O_CLOEXEC,
           (f2 & O_CLOEXEC) ? "在" : "不在（它属于 F_GETFD）");
    printf("      F_GETFL 里多出来的 0x8000：内核在 open 时会按平台附加 O_LARGEFILE\n");
    printf("        （fs/open.c:1443-1444: if (force_o_largefile()) flags |= O_LARGEFILE;）。\n");
    printf("        它不是你传的 —— 所以「F_GETFL 的返回值 ≠ 你传给 open 的 flags」。\n");
    printf("  没加的对照组       F_GETFD = 0x%x\n", fcntl(e2, F_GETFD));
    printf("  → FD_CLOEXEC 是「这个 fd 的」，dup 不会带走；O_* 状态标志是「打开描述」的。\n");
    close(e1);
    close(e2);
    unlink(path);
    return 0;
}
