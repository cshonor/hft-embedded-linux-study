/* c4_8_ioctl.c — ioctl()：通用模型之外的「控制通道」
 *
 * 演示五件事：
 *   1) 把 request 整数拆开看：方向 / size / type / nr 四个字段
 *   2) TIOCGWINSZ 查终端窗口大小——stdout 是管道时 ENOTTY
 *   3) isatty() 是怎么实现的（本质就是试一个 tty 专属 ioctl）
 *   4) FIONREAD 查「现在有多少字节可读」（通用命令，管道也能用）
 *   5) FIONBIO 用 ioctl 切非阻塞（与 fcntl 殊途同归，Ch5 详述）
 *
 * 编译: gcc -O0 -Wall -o c4_8_ioctl c4_8_ioctl.c
 */
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <sys/uio.h>

struct my_winsize { unsigned short ws_row, ws_col, ws_xpixel, ws_ypixel; };

int main(void)
{
    printf("== 1. request 整数里装了什么 ==\n");
    printf("  TIOCGWINSZ = 0x%08lx = %lu\n",
           (unsigned long) TIOCGWINSZ, (unsigned long) TIOCGWINSZ);
    printf("    按 asm-generic/ioctl.h 的位域拆：\n");
    printf("      dir  (2 bit, 位 30-31) = %lu\n",
           ((unsigned long) TIOCGWINSZ >> 30) & 0x3);
    printf("      size (14 bit, 位 16-29) = %lu   ← arg 结构体字节数\n",
           ((unsigned long) TIOCGWINSZ >> 16) & 0x3fff);
    printf("      type (8 bit, 位 8-15)  = %lu = '%c'   ← 设备类别「魔法数」\n",
           ((unsigned long) TIOCGWINSZ >> 8) & 0xff,
           (int) (((unsigned long) TIOCGWINSZ >> 8) & 0xff));
    printf("      nr   (8 bit, 位 0-7)   = %lu (0x%lx)   ← 命令序号\n",
           (unsigned long) TIOCGWINSZ & 0xff, (unsigned long) TIOCGWINSZ & 0xff);

    printf("\n  ⚠️ 注意 dir 和 size 都是 0 —— 它**没有**按 _IOR/_IOW 的规矩编码！\n");
    printf("     TIOCGWINSZ 是 tty 的**老式**命令字，内核里硬编码：\n");
    printf("       include/uapi/asm-generic/ioctls.h:  #define TIOCGWINSZ  0x5413\n");
    printf("     它恰好等于「无参数」写法 _IO('T', 0x13)：\n");
    printf("       _IO('T', 0x13) = 0x%08lx\n", (unsigned long) _IO('T', 0x13));
    printf("     若按网上流传的 `_IOR('T', 104, struct winsize)` 算，得到的是\n");
    printf("       0x%08lx —— 跟真实值对不上。那条说法是错的。\n",
           (unsigned long) _IOR('T', 104, struct my_winsize));
    printf("     tty 的一大批命令字都沿用 0x54xx 这段历史编号，别指望它们都合规。\n");

    printf("\n== 2. TIOCGWINSZ 查窗口大小 ==\n");
    struct winsize ws;
    errno = 0;
    int rc = ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    if (rc == 0 && ws.ws_row != 0)
        printf("  stdout 是终端：%u 行 x %u 列（像素 %u x %u）\n",
               ws.ws_row, ws.ws_col, ws.ws_xpixel, ws.ws_ypixel);
    else
        printf("  ioctl(stdout, TIOCGWINSZ) -> %d  errno=%d (%s)\n"
               "  → stdout 被重定向/接管了，不是终端，所以这个 tty 专属命令不认。\n",
               rc, errno, strerror(errno));

    printf("\n== 3. isatty() 的真相 ==\n");
    printf("  isatty(STDIN_FILENO)  = %d\n", isatty(STDIN_FILENO));
    printf("  isatty(STDOUT_FILENO) = %d\n", isatty(STDOUT_FILENO));
    printf("  isatty(open(\"/tmp\")) = %d\n", isatty(open("/tmp/c4_ioctl.txt", O_CREAT | O_WRONLY, 0644)));
    printf("  → glibc 的 isatty() 内部就是 ioctl(fd, TCGETS, &t)：能成功就是终端，\n");
    printf("    失败(ENOTTY)就不是。标准库把 ioctl 用在了最基础的判断上。\n");

    printf("\n== 4. FIONREAD：现在有多少字节可读 ==\n");
    int p[2];
    pipe(p);
    write(p[1], "123456789", 9);
    int avail = 0;
    rc = ioctl(p[0], FIONREAD, &avail);
    printf("  写 9 字节后 FIONREAD: 返回值=%d, avail=%d\n", rc, avail);
    char tmp[16];
    read(p[0], tmp, 4);
    ioctl(p[0], FIONREAD, &avail);
    printf("  读走 4 字节后再查:     avail=%d  ← 读走多少就少多少\n", avail);
    close(p[0]);
    close(p[1]);

    printf("\n== 5. FIONBIO：用 ioctl 切非阻塞 ==\n");
    int q[2];
    pipe(q);
    int on = 1;
    ioctl(q[0], FIONBIO, &on);                 /* 同 fcntl(q[0], F_SETFL, O_NONBLOCK) */
    errno = 0;
    ssize_t n = read(q[0], tmp, 4);            /* 管道是空的 */
    printf("  设为非阻塞后读空管道 -> %zd  errno=%d (%s)  ← 不阻塞，直接报「稍后再试」\n",
           n, errno, strerror(errno));
    close(q[0]);
    close(q[1]);

    printf("\n== 6. 普通文件上的 ioctl：ENOTTY ==\n");
    int f = open("/tmp/c4_ioctl.txt", O_RDONLY);
    errno = 0;
    rc = ioctl(f, TIOCGWINSZ, &ws);
    printf("  ioctl(普通文件, TIOCGWINSZ) -> %d  errno=%d (%s)\n",
           rc, errno, strerror(errno));
    printf("  → ENOTTY 的字面是 \"not a typewriter\"，历史遗留；\n");
    printf("    现代含义是「这个对象不认识这个命令」。\n");
    close(f);
    unlink("/tmp/c4_ioctl.txt");
    return 0;
}
