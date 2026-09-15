/* c14_5_vfs.c —— TLPI §14.5 The Virtual File System (VFS)
 *
 *  §14.5 那张「四对象」表（superblock / inode / dentry / file）是内核态概念，
 *  用户态没有直接 API。本 demo 做的是**找到每个对象的用户态投影**，
 *  再用实验把最容易混淆的一对拆开：
 *
 *    superblock ↔ statfs()/statvfs() 的整组字段 + /proc/self/mountinfo 的一条
 *    inode      ↔ stat() 的 st_ino / st_mode / st_nlink / st_size
 *    dentry     ↔ 无用户态 API；mountinfo 的 root=/mount_point= 是它最接近的投影
 *    file       ↔ fd（+ fcntl(F_GETFL) / F_GETFD）
 *
 *  核心实验：**「两个 fd」不等于「两个 file」**
 *    ① open() 两次同一个路径 → 两个结构性独立的 struct file（偏移各自独立）
 *    ② dup()  同一个 fd        → 两个 fd 指向**同一个** struct file（偏移共享）
 *    ③ open() 两个硬链接路径   → 两个 struct file，但指向**同一个 inode**
 *  判据用 lseek() 观察偏移，而不是猜。
 *
 *  编译：gcc -O0 -Wall -Wextra -o c14_5 c14_5_vfs.c
 */
#define _GNU_SOURCE
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void sec(const char *t)
{
    printf("\n== %s ==\n", t);
}

/* 打一个 fd 的三件套：fd 号、它自己的偏移、它指向的 inode */
static void show_fd(const char *tag, int fd)
{
    struct stat st;
    off_t off = lseek(fd, 0, SEEK_CUR);

    if (fstat(fd, &st) == -1) {
        printf("  %-26s fstat 失败 errno=%d\n", tag, errno);
        return;
    }
    printf("  %-26s fd=%d  offset=%-4lld ino=%llu flags=%#x\n",
           tag, fd, (long long) off, (unsigned long long) st.st_ino,
           (unsigned) fcntl(fd, F_GETFL));
}

/* 走一遍「fd → 路径」的反向投影（/proc/self/fd/N 是符号链接） */
static void show_fd_link(int fd)
{
    char p[64], buf[512];
    ssize_t n;

    snprintf(p, sizeof(p), "/proc/self/fd/%d", fd);
    n = readlink(p, buf, sizeof(buf) - 1);
    if (n < 0)
        printf("  %-26s readlink 失败 errno=%d\n", p, errno);
    else {
        buf[n] = '\0';
        printf("  %-26s -> %s\n", p, buf);
    }
}

int main(void)
{
    /* ---------- ① 四对象的用户态投影 ---------- */
    sec("① VFS 四对象的用户态投影");
    {
        struct statfs sfs;
        struct stat st;
        int fd = open("/etc/passwd", O_RDONLY);

        printf("  superblock -> statfs(\"/etc/passwd\").f_type = %#lx\n",
               statfs("/etc/passwd", &sfs) == 0 ? (unsigned long) sfs.f_type : 0UL);
        printf("              （同一个 superblock 的另一个投影在 /proc/self/mountinfo 的一条里）\n");

        if (fd == -1) {
            printf("  open(/etc/passwd) 失败 errno=%d (%s)\n", errno, strerror(errno));
            printf("\n=== c14_5 done ===\n");
            return 0;
        }
        if (fstat(fd, &st) == 0)
            printf("  inode      -> fstat(fd).st_ino = %llu, st_mode = %07o\n",
                   (unsigned long long) st.st_ino, (unsigned) st.st_mode & 077777);
        printf("  file       -> fd = %d, fcntl(F_GETFL) = %#x\n",
               fd, (unsigned) fcntl(fd, F_GETFL));
        printf("  dentry     -> 无用户态 API；/proc/self/fd/%d 的 readlink 是最接近的投影\n", fd);
        show_fd_link(fd);
        close(fd);
    }

    /* ---------- ② open 两次 = 两个 struct file ---------- */
    sec("② open() 两次同一路径 → 两个 struct file，偏移各自独立");
    {
        int fd1, fd2;
        ssize_t n;

        fd1 = open("/etc/passwd", O_RDONLY);
        fd2 = open("/etc/passwd", O_RDONLY);
        if (fd1 == -1 || fd2 == -1) {
            printf("  open 失败 errno=%d (%s)\n", errno, strerror(errno));
            if (fd1 != -1) close(fd1);
            if (fd2 != -1) close(fd2);
        } else {
            char buf[8];
            printf("  open 之前：\n");
            show_fd("fd1 (=open #1)", fd1);
            show_fd("fd2 (=open #2)", fd2);

            n = read(fd1, buf, 3);      /* 只推进 fd1 的偏移 */
            printf("  对 fd1 read 3 字节（返回 %zd）之后：\n", n);
            show_fd("fd1", fd1);
            show_fd("fd2", fd2);
            printf("  → fd1 移到 3，fd2 仍在 0：**两个独立的 struct file**\n");
            printf("    （但两者指向同一个 inode —— 见上面的 ino 相同）\n");
            close(fd1);
            close(fd2);
        }
    }

    /* ---------- ③ dup = 同一个 struct file ---------- */
    sec("③ dup() → 两个 fd 共用同一个 struct file（偏移共享）");
    {
        int fd1, fd2;
        ssize_t n;

        fd1 = open("/etc/passwd", O_RDONLY);
        fd2 = dup(fd1);
        if (fd1 == -1 || fd2 == -1) {
            printf("  open/dup 失败 errno=%d (%s)\n", errno, strerror(errno));
            if (fd1 != -1) close(fd1);
            if (fd2 != -1) close(fd2);
        } else {
            char buf[8];
            printf("  dup 之前：\n");
            show_fd("fd1 (=open)", fd1);
            show_fd("fd2 (=dup(fd1))", fd2);

            n = read(fd1, buf, 3);
            printf("  对 fd1 read 3 字节（返回 %zd）之后：\n", n);
            show_fd("fd1", fd1);
            show_fd("fd2", fd2);
            printf("  → 两个偏移**一起**移到 3：fd1 与 fd2 是同一个 struct file\n");
            printf("    这正是 dup2() 之后 close() 一个不影响另一个的原因（refcount）\n");
            close(fd1);
            close(fd2);
        }
    }

    /* ---------- ④ 两个路径 / 一个 inode ---------- */
    sec("④ 硬链接的两个路径 → 两个 struct file，但同一个 inode");
    {
        struct stat sa, sb;
        int fd1, fd2;

        unlink("/app/c14_5_a");
        unlink("/app/c14_5_b");

        fd1 = open("/app/c14_5_a", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd1 == -1) {
            printf("  open errno=%d (%s)\n", errno, strerror(errno));
        } else {
            ssize_t n = write(fd1, "0123456789", 10);
            printf("  写入 %zd 字节\n", n);
            close(fd1);

            if (link("/app/c14_5_a", "/app/c14_5_b") == 0) {
                fd1 = open("/app/c14_5_a", O_RDONLY);
                fd2 = open("/app/c14_5_b", O_RDONLY);
                fstat(fd1, &sa);
                fstat(fd2, &sb);
                printf("  fd1 -> /app/c14_5_a  ino=%llu\n", (unsigned long long) sa.st_ino);
                printf("  fd2 -> /app/c14_5_b  ino=%llu\n", (unsigned long long) sb.st_ino);
                printf("  → inode %s，但两个 struct file 各自有偏移\n",
                       sa.st_ino == sb.st_ino ? "相同" : "不同");
                if (lseek(fd1, 7, SEEK_SET) != (off_t) -1)
                    printf("  lseek(fd1, 7) 之后：fd1 offset=%lld，fd2 offset=%lld\n",
                           (long long) lseek(fd1, 0, SEEK_CUR),
                           (long long) lseek(fd2, 0, SEEK_CUR));
                close(fd1);
                close(fd2);
            }
            unlink("/app/c14_5_a");
            unlink("/app/c14_5_b");
        }
    }

    printf("\n=== c14_5 done ===\n");
    return 0;
}
