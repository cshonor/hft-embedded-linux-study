/* c4_2_universal.c — 「通用 I/O 模型」：同一套 open/read/write/close 通吃四种对象
 *
 * 四个目标：
 *   1) 普通文件        /tmp/c4_univ.txt
 *   2) 设备文件        /dev/null（写进去就消失）与 /dev/zero（读出来全是 0）
 *   3) 内核伪文件      /proc/version（内容由内核现场生成，没有磁盘块）
 *   4) 管道            pipe(2) 造出来的匿名管道
 *
 * 看点：调用形式完全一样，返回值/语义却不同——差异全被 VFS 的
 *       file->f_op 函数指针吸收了，用户态一行都不用改。
 *
 * 编译: gcc -O0 -Wall -o c4_2_universal c4_2_universal.c
 */
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

int main(void)
{
    char buf[128];
    ssize_t n;

    printf("== 1. 普通文件：写入后能读回来 ==\n");
    int fd = open("/tmp/c4_univ.txt", O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { perror("open file"); return 1; }
    printf("  写入 \"hello universal\" -> write 返回 %zd\n",
           write(fd, "hello universal", 15));
    lseek(fd, 0, SEEK_SET);                 /* 回到开头才能读到自己写的 */
    n = read(fd, buf, sizeof buf - 1);
    buf[n > 0 ? n : 0] = '\0';
    printf("  读回 %zd 字节: \"%s\"\n", n, buf);
    close(fd);

    printf("\n== 2. 设备文件 /dev/null：写进去就消失，读出来立刻 EOF ==\n");
    int dn = open("/dev/null", O_RDWR);
    if (dn < 0) { perror("open /dev/null"); return 1; }
    n = write(dn, "这段数据会被内核丢掉", 30);
    printf("  write 30 字节 -> %zd  （报告成功，但没有落点）\n", n);
    n = read(dn, buf, sizeof buf);
    printf("  read      -> %zd  （0 = EOF：永远读不到东西）\n", n);
    close(dn);

    printf("\n== 3. 内核伪文件 /proc/version：内容现场生成 ==\n");
    int pv = open("/proc/version", O_RDONLY);
    if (pv < 0) { perror("open /proc/version"); return 1; }
    n = read(pv, buf, sizeof buf - 1);
    buf[n > 0 ? n : 0] = '\0';
    printf("  read %zd 字节，开头是: %.60s...\n", n, buf);
    n = read(pv, buf, sizeof buf - 1);      /* 再读一次 */
    printf("  再 read -> %zd  （还不是 0：/proc/version 比这个 buffer 大）\n", n);
    n = read(pv, buf, sizeof buf - 1);      /* 第三次 */
    printf("  第三次 read -> %zd  （0 = 终于到 EOF）\n", n);
    printf("  → 连 proc 伪文件都必须「循环读到 0」，别假设一次就能读完。\n");
    close(pv);

    printf("\n== 4. 管道：两个 fd，一个读端一个写端，同一个 pipe(2) 造的 ==\n");
    int p[2];
    if (pipe(p) < 0) { perror("pipe"); return 1; }
    printf("  pipe() 给出读端 p[0]=%d 写端 p[1]=%d\n", p[0], p[1]);
    write(p[1], "via pipe", 8);
    n = read(p[0], buf, sizeof buf - 1);
    buf[n > 0 ? n : 0] = '\0';
    printf("  从 p[0] 读到 %zd 字节: \"%s\"\n", n, buf);

    printf("\n== 5. 差异在哪：同一个 read，四种对象走四个实现 ==\n");
    printf("  普通文件 read -> ext4/overlayfs 的 read_iter（走页缓存）\n");
    printf("  /dev/null  read -> 内核直接返回 0，没有任何存储\n");
    printf("  /proc/version -> procfs 的 read，现场格式化成字符串\n");
    printf("  管道        read -> 从内核环形缓冲区搬字节，读完即销毁\n");
    close(p[0]);
    close(p[1]);
    return 0;
}
