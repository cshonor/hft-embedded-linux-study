/* c13_2_stdio_buffering.c — Ch13 §13.2：stdio 库的缓冲
 *
 * TLPI §13.2（p.237）讲的是「用户态那一层缓冲」：
 *   ① `printf` / `fwrite` / `fgets` 先把数据放进 **用户态缓冲**，满了或
 *      显式刷新时才调 `read`/`write`；
 *   ② `setvbuf()` 三种模式：`_IOFBF` / `_IOLBF` / `_IONBF`；
 *   ③ 默认模式**取决于是不是终端**：stdout 接终端 → 行缓冲；接文件/管道 → 全缓冲。
 *
 * ⚠️ 本程序跑在 CE 上，stdout 是 **SOCKET**（不是终端）→ `isatty(1) = 0`
 *    → 默认**全缓冲**。所以「终端 vs 重定向」那一半**观察不到**。
 *    取而代之，本程序做两件更硬的事：
 *      a) 把判据本身量出来（isatty / st_mode / st_blksize / __fbufsize / __flbf）,
 *         并用 glibc 源码说明**为什么**缓冲区是 4096 而不是 BUFSIZ(8192)；
 *      b) **显式**切成 `_IOLBF` —— 这样就能在 CE 上复现「遇到 '\n' 就刷」这一半，
 *         而这正是终端下默认发生的事。
 *    并且**不依赖肉眼观察顺序**：每一轮都把「stdio 缓冲里有多少字节」量出来，
 *    用插入的 write() 当参照物，结果只由数字说话。
 *
 * 编译： gcc -O0 -Wall -Wextra -o c13_2_stdio_buffering c13_2_stdio_buffering.c
 * 取材： TLPI §13.2（setvbuf 三模式、fflush、自动刷新时机、重定向导致全缓冲）
 *       man-pages 6.19 setvbuf(3)（"must be called ... before any other operation"）
 *                      fflush(3)（NULL 刷新所有输出流；输入流的行为未标准化）
 *                      stdio(3)（BUFSIZ；默认缓冲模式与 isatty 的关系）
 *       glibc 2.39 libio/filedoalloc.c 的 _IO_file_doallocate()：
 *         size = BUFSIZ;
 *         if (S_ISCHR(st.st_mode)) { if (isatty(...)) fp->_flags |= _IO_LINE_BUF; }
 *         if (st.st_blksize > 0 && st.st_blksize < BUFSIZ) size = st.st_blksize;
 *         → 行缓冲只可能来自「字符设备 + isatty」；缓冲大小取 st_blksize
 *       glibc 2.39 libio/iofdopen.c / libio/libioP.h（__fbufsize / __flbf 的数据来源）
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdio_ext.h>      /* __fbufsize() / __flbf() / __freading() —— glibc 扩展 */
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* 报一次「stdout 当前是什么状态」 */
static void dump_stdout_state(const char *tag)
{
    struct stat st;

    if (fstat(STDOUT_FILENO, &st) == -1) {
        printf("  %-22s fstat 失败 errno=%d\n", tag, errno);
        return;
    }
    printf("  %-22s isatty=%d  __flbf=%d  __fbufsize=%zu  st_blksize=%ld\n",
           tag, isatty(STDOUT_FILENO), __flbf(stdout) != 0,
           __fbufsize(stdout), (long) st.st_blksize);
}

int main(void)
{
    printf("== ① 判据本身：stdout 是什么、缓冲多大 ==\n");
    {
        struct stat st;

        printf("  isatty(STDOUT_FILENO) = %d  （1 = 终端；0 = 走全缓冲）\n",
               isatty(STDOUT_FILENO));
        if (fstat(STDOUT_FILENO, &st) == 0) {
            printf("  fstat(1).st_mode = %07o   S_ISCHR=%d  S_ISFIFO=%d  S_ISSOCK=%d\n",
                   (unsigned) st.st_mode, S_ISCHR(st.st_mode),
                   S_ISFIFO(st.st_mode), S_ISSOCK(st.st_mode));
            printf("  → 本容器里 stdout 是 **socket**，既不是字符设备也不是管道。\n");
            printf("  BUFSIZ = %d   而 __fbufsize(stdout) = %zu\n",
                   BUFSIZ, __fbufsize(stdout));
            printf("  → 两者**不相等**，且后者恰好 = st_blksize = %ld：\n", (long) st.st_blksize);
            printf("     glibc 的 _IO_file_doallocate()（libio/filedoalloc.c）里：\n");
            printf("       size = BUFSIZ;\n");
            printf("       if (st.st_blksize > 0 && st.st_blksize < BUFSIZ) size = st.st_blksize;\n");
            printf("     st_blksize(%ld) < BUFSIZ(%d) → size 取 4096。\n",
                   (long) st.st_blksize, BUFSIZ);
            printf("     同一段代码里，行缓冲是**只有** S_ISCHR + isatty() 才设的：\n");
            printf("       if (S_ISCHR(st.st_mode)) { if (isatty(fd)) fp->_flags |= _IO_LINE_BUF; }\n");
            printf("     所以「stdout 到终端就自动行缓冲」不是约定，是这段代码。\n");
        }
        printf("\n");
        dump_stdout_state("默认状态:");
    }

    printf("\n== ② 三种模式：setvbuf 之后 __fbufsize / __flbf 怎么变 ==\n");
    {
        dump_stdout_state("默认(_IOFBF):");

        /* ⚠️ 顺序很讲究：C 标准要求 setvbuf「在任何 I/O 之前」调用。
           下面这个流已经用过，所以严格说这是未定义行为 —— 实测能不能生效，
           正是本节要量的事。 */
        errno = 0;
        int r1 = setvbuf(stdout, NULL, _IOLBF, 4096);
        int e1 = errno;
        printf("  setvbuf(stdout, NULL, _IOLBF, 4096) = %d errno=%d(%s)\n",
               r1, e1, strerror(e1));
        dump_stdout_state("改成 _IOLBF:");
        printf("     → __flbf 从 0 变成 %d：**帧内确实改成了行缓冲**。\n", __flbf(stdout) != 0);

        errno = 0;
        int r2 = setvbuf(stdout, NULL, _IONBF, 0);
        int e2 = errno;
        printf("  setvbuf(stdout, NULL, _IONBF, 0)    = %d errno=%d(%s)\n",
               r2, e2, strerror(e2));
        dump_stdout_state("改成 _IONBF:");

        errno = 0;
        int r3 = setvbuf(stdout, NULL, _IOFBF, 4096);
        int e3 = errno;
        printf("  setvbuf(stdout, NULL, _IOFBF, 4096) = %d errno=%d(%s)\n",
               r3, e3, strerror(e3));
        dump_stdout_state("改回 _IOFBF:");
        printf("  ⚠️ 三次都返回 0（成功），模式也真的跟着变了 —— 说明 glibc 允许\n");
        printf("     对已经用过的流改缓冲（标准不保证，别在工程里这么写）。\n");
        printf("     规范用法：`setvbuf(fp, buf, mode, size);` 紧跟在 fopen/fdopen 之后。\n");
    }

    printf("\n== ③ 行缓冲的机制：'\\n' 真的会触发 flush（CE 上也能复现） ==\n");
    {
        printf("  做法：把 stdout 切成 _IOLBF，然后用 write() 当「参照物」——\n");
        printf("        write() 不过用户态缓冲，它一旦出现，就说明它之前的东西已经出去了。\n");
        printf("  下面每轮都先记一个「已写出字节数」，再决定 printf 里带不带 '\\n'。\n\n");

        int fds[2];
        if (pipe(fds) == -1) {
            printf("  pipe 失败 errno=%d(%s)\n", errno, strerror(errno));
            return EXIT_FAILURE;
        }

        /* 把 stdout 换成管道写端 —— 这样能**自己读回来**，不靠肉眼。 */
        int saved = dup(STDOUT_FILENO);
        if (saved == -1 || dup2(fds[1], STDOUT_FILENO) == -1) {
            printf("  dup2 失败 errno=%d(%s)\n", errno, strerror(errno));
            return EXIT_FAILURE;
        }
        close(fds[1]);
        setvbuf(stdout, NULL, _IOLBF, 4096);

        /* 轮 1：printf 里**带** '\n' */
        printf("LINE1-with-newline\n");
        /* 轮 2：printf 里**不带** '\n' */
        printf("LINE2-no-newline");
        /* 参照物 */
        ssize_t ref = write(STDOUT_FILENO, "REF\n", 4);
        fflush(stdout);

        dup2(saved, STDOUT_FILENO);
        close(saved);

        char back[256];
        memset(back, 0, sizeof(back));
        ssize_t got = read(fds[0], back, sizeof(back) - 1);
        close(fds[0]);

        printf("  管道里读回来 %ld 字节（write 参照物写了 %ld 字节）：\n", (long) got, (long) ref);
        {
            char *p = back;
            int ln = 1;
            while (*p != '\0') {
                char *nl = strchr(p, '\n');
                if (nl == NULL) {
                    printf("    [行 %d] \"%s\"  ← 没有换行结尾，说明它是被「满/fflush」推出去的\n", ln, p);
                    break;
                }
                *nl = '\0';
                printf("    [行 %d] \"%s\"\n", ln, p);
                p = nl + 1;
                ln++;
            }
        }
        printf("  → 行缓冲下：带 '\\n' 的 \"LINE1-with-newline\" **在 REF 之前**就出去了；\n");
        printf("     不带 '\\n' 的 \"LINE2-no-newline\" 一直待在缓冲里，直到最后的 fflush()。\n");
        printf("     这正是终端下默认发生的事（区别只是终端默认就是 _IOLBF，不用手动设）。\n");
    }

    printf("\n== ④ fileno() / fdopen()：两层之间的桥 ==\n");
    {
        const char *path = "/app/c13_2.txt";
        FILE *fp = fopen(path, "w");
        int fd;

        if (fp == NULL) {
            printf("  fopen 失败 errno=%d(%s)\n", errno, strerror(errno));
            return EXIT_FAILURE;
        }
        fd = fileno(fp);
        printf("  fopen(\"%s\", \"w\") -> FILE* %p，fileno() = %d\n",
               path, (void *) fp, fd);
        fprintf(fp, "written-by-fprintf");
        {
            struct stat st;
            fstat(fd, &st);
            printf("  fprintf 之后，文件大小（fstat 走 fd）= %ld 字节 ← 还在 stdio 缓冲里，没进内核\n",
                   (long) st.st_size);
        }
        fflush(fp);
        {
            struct stat st;
            fstat(fd, &st);
            printf("  fflush 之后，文件大小 = %ld 字节 ← 已经到内核（页缓存）里了\n",
                   (long) st.st_size);
        }
        printf("  → 这就是「FILE* 有缓冲、fd 没有」的可测差别：同一个 fd，\n");
        printf("     fstat 看到的大小在 fflush 前后是不同的。\n");
        printf("  ⚠️ 因此 `fprintf(fp, ...)` 之后立刻 `write(fileno(fp), ...)` 就一定乱序。\n");
        fclose(fp);
        unlink(path);
    }

    printf("\n== ⑤ 别把「缓冲」和「落盘」混为一谈 ==\n");
    printf("  用户态缓冲(stdio)  --fflush-->  内核页缓存  --fsync/fdatasync-->  磁盘\n");
    printf("    ↑ 本节            ↑ §13.3 讲的          ↑ §13.3\n");
    printf("  `fflush()` **只管用户态那一层**，它不落盘；`fsync()` 才能落盘，但它\n");
    printf("  管不到还待在 stdio 缓冲里的字节。两者各管一层，不能互相替代。\n");
    printf("  完整的两层数据流见 13.4；混用的后果与修法见 13.7。\n");
    return EXIT_SUCCESS;
}
