/* TLPI 第 2 章 §2.4 —— 单根目录树：硬链接 vs 软链接，7 种文件类型
 *
 * 编译：gcc -O0 -Wall -Wextra c2_4_links.c -o c2_4
 * 运行：./c2_4
 *
 * 本节要钉死的事实：
 *   ① 文件名不在 inode 里，存在「目录的数据块」= 一张「名字 → inode 号」映射表。
 *   ② 硬链接 = 同一 inode 的另一个名字（nlink 加一）；软链接 = 独立 inode，内容是路径串。
 *   ③ 删掉原文件：硬链接仍可访问（inode 引用计数 > 0），软链接悬空（dangling）。
 *   ④ 硬链接不能跨文件系统（EXDEV），软链接可以。
 *   ⑤ Linux 的 7 种文件类型用 stat 的 st_mode 配合 S_IS* 宏判断。
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define DIR  "/tmp/c2_4"

static void jb(const char *label, int cond, const char *true_s, const char *false_s)
{
    printf("  %-46s %s\n", label, cond ? true_s : false_s);
}

/* 给一个路径判类型（stat 跟随符号链接，lstat 不跟随） */
static char type_char(mode_t m)
{
    if (S_ISREG(m))  return '-';
    if (S_ISDIR(m))  return 'd';
    if (S_ISLNK(m))  return 'l';
    if (S_ISCHR(m))  return 'c';
    if (S_ISBLK(m))  return 'b';
    if (S_ISFIFO(m)) return 'p';
    if (S_ISSOCK(m)) return 's';
    return '?';
}

static const char *type_name(mode_t m)
{
    if (S_ISREG(m))  return "普通文件";
    if (S_ISDIR(m))  return "目录";
    if (S_ISLNK(m))  return "符号链接";
    if (S_ISCHR(m))  return "字符设备";
    if (S_ISBLK(m))  return "块设备";
    if (S_ISFIFO(m)) return "FIFO(命名管道)";
    if (S_ISSOCK(m)) return "套接字";
    return "未知";
}

int main(void)
{
    char orig[128], hard[128], sym[128];
    snprintf(orig, sizeof(orig), "%s/orig.txt", DIR);
    snprintf(hard, sizeof(hard), "%s/hard.txt", DIR);
    snprintf(sym,  sizeof(sym),  "%s/sym.txt",  DIR);

    /* 干净起步 */
    unlink(hard); unlink(sym); unlink(orig);
    mkdir(DIR, 0755);

    printf("=== ① 建三种名字 ===\n");
    int fd = open(orig, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { perror("open orig"); return 1; }
    if (write(fd, "hello inode\n", 12) != 12) perror("write");
    close(fd);
    printf("  open+write 建好了 %s\n", orig);

    if (link(orig, hard) == 0)       printf("  link(\"%s\", \"%s\") 成功\n", orig, hard);
    else                             printf("  link 失败 errno=%d(%s)\n", errno, strerror(errno));
    if (symlink(orig, sym) == 0)     printf("  symlink(\"%s\", \"%s\") 成功（存的是路径串）\n", orig, sym);
    else                             printf("  symlink 失败 errno=%d(%s)\n", errno, strerror(errno));
    errno = 0;

    printf("\n=== ② inode 号与链接计数（stat 跟随 / lstat 不跟随）===\n");
    struct stat a, h, l;
    if (stat(orig, &a) != 0) { perror("stat orig"); return 1; }
    if (stat(hard, &h) != 0) { perror("stat hard"); return 1; }
    if (lstat(sym, &l) != 0) { perror("lstat sym"); return 1; }

    printf("  %-12s st_ino=%-10ld st_nlink=%ld  type=%c(%s)\n",
           "orig.txt", (long)a.st_ino, (long)a.st_nlink, type_char(a.st_mode), type_name(a.st_mode));
    printf("  %-12s st_ino=%-10ld st_nlink=%ld  type=%c(%s)\n",
           "hard.txt", (long)h.st_ino, (long)h.st_nlink, type_char(h.st_mode), type_name(h.st_mode));
    printf("  %-12s st_ino=%-10ld st_nlink=%ld  type=%c(%s)  size=%ld\n",
           "sym.txt", (long)l.st_ino, (long)l.st_nlink, type_char(l.st_mode),
           type_name(l.st_mode), (long)l.st_size);

    printf("\n  关系判定（这部分每次跑都一样）：\n");
    jb("硬链接与原文件 inode 相同？", a.st_ino == h.st_ino, "SAME（同一 inode）", "DIFFERENT");
    jb("软链接与原文件 inode 相同？", l.st_ino != a.st_ino, "DIFFERENT（独立 inode）", "SAME");
    jb("硬链接使 nlink 增到 2？", a.st_nlink == 2 && h.st_nlink == 2, "YES nlink=2", "NO");
    jb("软链接的 st_size == 原路径长度？",
       (long)l.st_size == (long)strlen(orig), "YES（内容就是路径串）", "NO");

    printf("\n=== ③ 软链接与硬链接的「追不动」实验 ===\n");
    printf("  stat(\"sym.txt\") 跟随链接 -> 落在目标 orig.txt 上：\n");
    struct stat s2;
    if (stat(sym, &s2) == 0)
        printf("    stat  ok  st_ino=%ld  type=%c(%s)\n",
               (long)s2.st_ino, type_char(s2.st_mode), type_name(s2.st_mode));
    printf("  lstat(\"sym.txt\") 不跟随 -> 看到链接自己：\n");
    if (lstat(sym, &s2) == 0)
        printf("    lstat ok  st_ino=%ld  type=%c(%s)\n",
               (long)s2.st_ino, type_char(s2.st_mode), type_name(s2.st_mode));

    printf("\n=== ④ 删掉原文件之后 ===\n");
    printf("  unlink(\"%s\")\n", orig);
    unlink(orig);
    struct stat h2;
    errno = 0;
    if (stat(hard, &h2) == 0)
        printf("  硬链接 hard.txt 仍可 stat：st_ino=%ld nlink=%ld  -> 数据还活着\n",
               (long)h2.st_ino, (long)h2.st_nlink);
    else
        printf("  硬链接 stat 失败 errno=%d(%s)\n", errno, strerror(errno));
    errno = 0;
    if (stat(sym, &s2) == 0)
        printf("  软链接 stat 竟然成功（不该发生）\n");
    else
        printf("  软链接 stat 失败 errno=%d(%s)  -> 悬空(dangling)\n",
               errno, strerror(errno));
    errno = 0;
    if (lstat(sym, &s2) == 0)
        printf("  但 lstat(sym) 仍成功：链接文件本体现在还在，type=%c\n",
               type_char(s2.st_mode));
    printf("  把 nlink 归零后 inode 才真正释放：\n");
    unlink(hard);
    printf("    unlink(hard.txt) 之后，这个 inode 的引用计数归零\n");

    printf("\n=== ⑤ 硬链接不能跨文件系统 ===\n");
    errno = 0;
    if (link(sym, "/app/c2_4_xlink") == 0) {
        printf("  link(/tmp/... -> /app/...) 竟然成功 -> 说明 /tmp 与 /app 同一个 FS\n");
        unlink("/app/c2_4_xlink");
    } else {
        printf("  link(/tmp/... -> /app/...) 失败 errno=%d(%s)\n", errno, strerror(errno));
        printf("  EEXIST(%d)=目标已存在  EXDEV(%d)=跨文件系统\n", EEXIST, EXDEV);
    }
    errno = 0;

    printf("\n=== ⑥ 7 种文件类型一次认全 ===\n");
    /* 造一个 FIFO 和一个已绑定的 UNIX 套接字 */
    char fifo[128], sockp[128];
    snprintf(fifo, sizeof(fifo), "%s/pipe.fifo", DIR);
    snprintf(sockp, sizeof(sockp), "%s/app.sock", DIR);
    unlink(fifo); unlink(sockp);
    mkfifo(fifo, 0644);
    int sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un sa; memset(&sa, 0, sizeof(sa));
    sa.sun_family = AF_UNIX;
    strncpy(sa.sun_path, sockp, sizeof(sa.sun_path) - 1);
    if (sfd >= 0) bind(sfd, (struct sockaddr *)&sa, sizeof(sa));

    struct { const char *p; } cand[] = {
        { "/proc/self/exe" }, { DIR }, { sym }, { "/dev/null" },
        { "/dev/zero" }, { fifo }, { sockp }, { "/dev" },
    };
    printf("  %-24s %-4s %-14s %s\n", "路径", "类型", "S_ISxx", "说明");
    printf("  %-24s %-4s %-14s %s\n", "----------------------", "---",
           "------------", "----");
    for (unsigned i = 0; i < sizeof(cand) / sizeof(cand[0]); i++) {
        struct stat st;
        if (lstat(cand[i].p, &st) != 0) {
            printf("  %-24s %-4s %-14s (lstat 失败 errno=%d)\n",
                   cand[i].p, "?", "-", errno);
            errno = 0;
            continue;
        }
        const char *macro = S_ISREG(st.st_mode)  ? "S_ISREG"
                          : S_ISDIR(st.st_mode)  ? "S_ISDIR"
                          : S_ISLNK(st.st_mode)  ? "S_ISLNK"
                          : S_ISCHR(st.st_mode)  ? "S_ISCHR"
                          : S_ISBLK(st.st_mode)  ? "S_ISBLK"
                          : S_ISFIFO(st.st_mode) ? "S_ISFIFO"
                          : S_ISSOCK(st.st_mode) ? "S_ISSOCK" : "?";
        printf("  %-24s %-4c %-14s %s\n", cand[i].p, type_char(st.st_mode),
               macro, type_name(st.st_mode));
    }
    printf("  块设备(b)本环境没有；容器里 /dev 只映射了 null/zero/random/urandom。\n");

    if (sfd >= 0) close(sfd);
    unlink(sockp); unlink(fifo);
    return 0;
}
