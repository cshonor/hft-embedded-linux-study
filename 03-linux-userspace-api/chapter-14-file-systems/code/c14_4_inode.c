/* c14_4_inode.c —— TLPI §14.4 I-nodes
 *
 *  一节的三个硬结论，各配一个可观测实验：
 *    ① inode 号**只在同一个文件系统内唯一** —— 不同 FS 上可以重复
 *       实验：同时 stat("/") 与 stat("/proc")，两者的 inode 号可以一模一样
 *    ② 文件名在目录的数据块里，不在 inode 里；硬链接就是「目录里两条名字
 *       指向同一个 inode」 —— 实验：link() 前后 st_nlink 与 st_ino
 *    ③ inode 里的数据块指针决定「逻辑大小 ≠ 物理占用」
 *       实验：st_size 与 st_blocks 的对比（稀疏文件、以及小文件的对齐开销）
 *       再加一个「unlink 之后 fd 还能读」——证明释放发生在「链接计数为 0
 *       且无人打开」之后
 *
 *  编译：gcc -O0 -Wall -Wextra -o c14_4 c14_4_inode.c
 */
#define _GNU_SOURCE
#include <sys/stat.h>
#include <sys/sysmacros.h>
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

/* 打 inode 的全字段（TLPI §14.4 的那张表） */
static void show_inode(const char *path)
{
    struct stat st;
    const char *kind;

    if (stat(path, &st) == -1) {
        printf("  stat(%s) errno=%d (%s)\n", path, errno, strerror(errno));
        return;
    }
    kind = S_ISDIR(st.st_mode) ? "dir" : S_ISREG(st.st_mode) ? "reg" :
           S_ISLNK(st.st_mode) ? "lnk" : S_ISCHR(st.st_mode) ? "chr" :
           S_ISBLK(st.st_mode) ? "blk" : S_ISFIFO(st.st_mode) ? "fifo" :
           S_ISSOCK(st.st_mode) ? "sock" : "???";
    printf("  %s\n", path);
    printf("    st_dev=%u:%-5u (设备号，同一 FS 内相同，用来给 inode 号「定域」)\n",
           major(st.st_dev), minor(st.st_dev));
    printf("    st_ino=%-12llu (inode 号)\n", (unsigned long long) st.st_ino);
    printf("    st_mode=%07o %-4s (S_IFMT + 权限位)\n",
           (unsigned) st.st_mode & 077777, kind);
    printf("    st_nlink=%-11lu (硬链接计数)\n", (unsigned long) st.st_nlink);
    printf("    st_size=%-12lld st_blocks=%-8lld (× 512 字节 = %lld 字节物理占用)\n",
           (long long) st.st_size, (long long) st.st_blocks,
           (long long) st.st_blocks * 512);
    printf("    st_blksize=%-9lu (该 FS 的「推荐 I/O 块大小」)\n",
           (unsigned long) st.st_blksize);
}

int main(void)
{
    /* ---------- ① inode 号的作用域 ---------- */
    sec("① inode 号只在同一个文件系统内唯一");
    {
        struct stat a, b, c;
        int r1 = stat("/", &a);
        int r2 = stat("/proc", &b);
        int r3 = stat("/app", &c);

        if (r1 == 0 && r2 == 0) {
            printf("  /      : st_dev=%u:%-5u st_ino=%llu\n",
                   major(a.st_dev), minor(a.st_dev), (unsigned long long) a.st_ino);
            printf("  /proc  : st_dev=%u:%-5u st_ino=%llu\n",
                   major(b.st_dev), minor(b.st_dev), (unsigned long long) b.st_ino);
            printf("  → 两个 inode 号%s，但 st_dev 不同 → **不是同一个对象**\n",
                   a.st_ino == b.st_ino ? "相同" : "不同");
            printf("    inode 号必须配上 st_dev 才有意义，这就是硬链接不能跨 FS 的原因。\n");
        }
        if (r3 == 0)
            printf("  /app   : st_dev=%u:%-5u st_ino=%llu\n",
                   major(c.st_dev), minor(c.st_dev), (unsigned long long) c.st_ino);

        printf("\n  把一个真实文件的 inode 全字段摊开（就是 TLPI §14.4 的那张表）：\n");
        show_inode("/etc/passwd");
    }

    /* ---------- ② 硬链接 ---------- */
    sec("② 硬链接 = 目录里的第二条名字指向同一个 inode");
    {
        struct stat st;
        int fd;

        unlink("/app/c14_4_a");
        unlink("/app/c14_4_b");

        fd = open("/app/c14_4_a", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            printf("  open errno=%d (%s)\n", errno, strerror(errno));
        } else {
            ssize_t n = write(fd, "hello inode\n", 12);
            printf("  写入 %zd 字节到 /app/c14_4_a\n", n);
            close(fd);

            stat("/app/c14_4_a", &st);
            printf("  link 前: ino=%-10llu nlink=%lu\n",
                   (unsigned long long) st.st_ino, (unsigned long) st.st_nlink);

            if (link("/app/c14_4_a", "/app/c14_4_b") == -1) {
                printf("  link errno=%d (%s)\n", errno, strerror(errno));
            } else {
                struct stat s2;
                stat("/app/c14_4_a", &st);
                stat("/app/c14_4_b", &s2);
                printf("  link 后: a  ino=%-10llu nlink=%lu\n",
                       (unsigned long long) st.st_ino, (unsigned long) st.st_nlink);
                printf("           b  ino=%-10llu nlink=%lu\n",
                       (unsigned long long) s2.st_ino, (unsigned long) s2.st_nlink);
                printf("  → 两条路径 ino %s，nlink 从 1 变 2；**没有复制数据块**\n",
                       st.st_ino == s2.st_ino ? "相同" : "不同");

                /* 跨 FS 硬链接 */
                errno = 0;
                if (link("/app/c14_4_a", "/tmp/c14_4_c") == -1)
                    printf("  link 到 /tmp（另一个 FS）-> -1 errno=%d (%s)\n",
                           errno, strerror(errno));
                else
                    printf("  link 到 /tmp -> 0（意外）\n");
            }

            /* 删掉一条名字 */
            unlink("/app/c14_4_b");
            stat("/app/c14_4_a", &st);
            printf("  unlink(\"b\") 后: nlink=%lu（数据一个字节都没动）\n",
                   (unsigned long) st.st_nlink);

            /* ---------- ③ unlink 之后 fd 还能读 ---------- */
            sec("③ unlink 之后：名字没了，已打开的 fd 仍能读");
            fd = open("/app/c14_4_a", O_RDONLY);
            if (fd != -1) {
                char buf[32];
                ssize_t n;

                unlink("/app/c14_4_a");
                if (stat("/app/c14_4_a", &st) == -1)
                    printf("  unlink(\"a\") 后 stat 失败 errno=%d (%s) —— 名字没了\n",
                           errno, strerror(errno));
                n = read(fd, buf, sizeof(buf) - 1);
                if (n > 0) {
                    buf[n] = '\0';
                    printf("  但**已打开的 fd 仍能读**：读回 %zd 字节 \"%s\"\n", n,
                           strcmp(buf, "hello inode\n") == 0 ? "hello inode" : buf);
                }
                close(fd);
                printf("  → 真正的数据块释放条件是：nlink == 0 **且** 没有打开的 fd\n");
            }
        }
    }

    /* ---------- ④ st_size vs st_blocks ---------- */
    sec("④ st_size（逻辑大小）vs st_blocks（物理占用，512 字节单位）");
    {
        struct stat st;
        int fd;
        const char *p = "/app/c14_4_size";

        unlink(p);
        fd = open(p, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd != -1) {
            /* 只写 1 个字节 —— ext4 会按块分配，所以物理占用远大于 1 */
            ssize_t n = write(fd, "X", 1);
            close(fd);
            stat(p, &st);
            printf("  写 1 个字节   : st_size=%-6lld st_blocks=%-4lld 物理 %lld 字节  (%zd)\n",
                   (long long) st.st_size, (long long) st.st_blocks,
                   (long long) st.st_blocks * 512, n);
        }

        unlink(p);
        /* 稀疏文件：lseek 跳过 1 MiB 再写 1 个字节 */
        fd = open(p, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd != -1) {
            off_t off = lseek(fd, 1024 * 1024, SEEK_SET);
            ssize_t n = write(fd, "Y", 1);
            close(fd);
            stat(p, &st);
            printf("  稀疏文件      : st_size=%-6lld st_blocks=%-4lld 物理 %lld 字节  (lseek→%lld, write=%zd)\n",
                   (long long) st.st_size, (long long) st.st_blocks,
                   (long long) st.st_blocks * 512, (long long) off, n);
            printf("  → 逻辑大小 1 MiB+1，物理只有最后一小块 —— 中间的洞没有分配块\n");
        }

        /* 把小文件读一遍，看物理量有没有变化（至少能说明丢零/预读机制存在） */
        unlink(p);
        fd = open(p, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd != -1) {
            char buf[4096];
            memset(buf, 'Z', sizeof(buf));
            ssize_t n = write(fd, buf, sizeof(buf));
            close(fd);
            stat(p, &st);
            printf("  写 4096 字节  : st_size=%-6lld st_blocks=%-4lld 物理 %lld 字节  (write=%zd)\n",
                   (long long) st.st_size, (long long) st.st_blocks,
                   (long long) st.st_blocks * 512, n);
        }
        unlink(p);
    }

    printf("\n=== c14_4 done ===\n");
    return 0;
}
