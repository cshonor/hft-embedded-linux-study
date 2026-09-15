/* c4_6_close.c — close() 的三个真相：回收 fd 号、减引用计数、不保证落盘
 *
 * 演示四件事：
 *   1) close 之后那个 fd 号立刻可被复用（所以「用旧 fd」是致命错误）
 *   2) 重复 close / close 非法 fd -> EBADF
 *   3) dup 出来的两个 fd 共用一个「打开文件描述」，close 一个不影响另一个
 *   4) fork 之后父子共享同一个打开描述（引用计数 +1）
 *
 * 编译: gcc -O0 -Wall -o c4_6_close c4_6_close.c
 */
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

int main(void)
{
    char c;
    const char *path = "/tmp/c4_close.txt";

    printf("== 1. close 后 fd 号立即被回收 ==\n");
    int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    write(fd, "ABCDEFGHIJ", 10);
    printf("  打开 -> fd=%d\n", fd);
    close(fd);
    printf("  close(%d) 完成\n", fd);

    int other = open(path, O_RDONLY);
    printf("  再打开同一个文件 -> fd=%d  （%s）\n", other,
           other == fd ? "复用了刚释放的号" : "没复用");
    printf("  ⚠️ 此刻若还用旧的 fd=%d 去读写，操作的是「other」这个文件\n", fd);
    printf("     （内核不会报错——号还是合法的，只是指向变了）。\n");

    printf("\n== 2. close 非法 fd / 重复 close ==\n");
    errno = 0;
    int rc = close(999);
    printf("  close(999) -> %d  errno=%d (%s)\n", rc, errno, strerror(errno));
    errno = 0;
    rc = close(other);
    printf("  close(%d) 第一次 -> %d  errno=%d (%s)\n", other, rc, errno, strerror(errno));
    errno = 0;
    rc = close(other);
    printf("  close(%d) 第二次 -> %d  errno=%d (%s)  ← 重复关闭\n",
           other, rc, errno, strerror(errno));

    printf("\n== 3. dup 共享打开文件描述：close 一个，另一个照用 ==\n");
    int a = open(path, O_RDONLY);
    int b = dup(a);
    printf("  a=%d, dup 得 b=%d（同一个 struct file，引用计数 = 2）\n", a, b);
    lseek(a, 3, SEEK_SET);
    read(a, &c, 1);
    printf("  用 a 读到偏移 3 的 '%c'，此时 a 的偏移 = %ld\n",
           c, (long) lseek(a, 0, SEEK_CUR));
    printf("  用 b 查偏移           = %ld  ← 共享！同一个 f_pos\n",
           (long) lseek(b, 0, SEEK_CUR));
    close(a);
    printf("  close(a) 之后：\n");
    ssize_t rn = read(b, &c, 1);            /* 单独一行：别把有副作用的调用塞进 printf 参数 */
    printf("    用 b 还能读吗？read(b) -> %zd 字节，读到 '%c'\n", rn, c);
    printf("  → close 只是让引用计数减一，没减到 0 就不释放 struct file。\n");
    close(b);

    printf("\n== 4. fork 之后父子的偏移也共享 ==\n");
    int f = open(path, O_RDONLY);
    lseek(f, 0, SEEK_SET);
    pid_t pid = fork();
    if (pid == 0) {
        /* 子进程：继承 f，读 4 字节 */
        char cb[5];
        ssize_t n = read(f, cb, 4);
        cb[n > 0 ? n : 0] = '\0';
        _exit(0);
    }
    waitpid(pid, NULL, 0);
    printf("  子进程读了 4 字节后，父进程里 f 的偏移 = %ld\n",
           (long) lseek(f, 0, SEEK_CUR));
    printf("  → 4，不是 0。fork 复制的是 fd 表，表项指向同一个 struct file。\n");
    printf("    父子共享偏移 = 多进程写同一日志会交错，这才是 O_APPEND 存在的理由。\n");
    close(f);
    unlink(path);
    return 0;
}
