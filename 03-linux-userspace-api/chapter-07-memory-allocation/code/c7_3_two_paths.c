/* Ch7 §7.1 — malloc 的两条路：扩主堆 [heap]，还是新开一段 mmap
 * 编译: gcc -O2 -Wall -Wextra -o c7_3_two_paths c7_3_two_paths.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

/* 直接 open/read 读 /proc/self/maps。
 * 不用 fopen —— 它自己会 malloc 一块缓冲，反而污染我们要观测的堆。 */
static long maps_kb(int which)          /* which: 0 = 匿名段, 1 = [heap] */
{
    int fd = open("/proc/self/maps", O_RDONLY);
    if (fd < 0) return -1;

    static char buf[65536];             /* 静态区，不占堆 */
    ssize_t n = read(fd, buf, sizeof buf - 1);
    close(fd);
    if (n <= 0) return -1;
    buf[n] = '\0';

    long kb = 0;
    char *line = buf;
    while (line != NULL && *line != '\0') {
        char *nl = strchr(line, '\n');
        if (nl != NULL) *nl = '\0';

        unsigned long a = 0, b = 0;
        if (sscanf(line, "%lx-%lx", &a, &b) == 2) {
            long size = (long) ((b - a) / 1024);
            if (which == 1) {
                if (strstr(line, "[heap]") != NULL) kb += size;
            } else {
                char *last = strrchr(line, ' ');
                if (last == NULL || *(last + 1) == '\0')
                    kb += size;         /* 没有名字 = 匿名映射 */
            }
        }
        line = (nl != NULL) ? nl + 1 : NULL;
    }
    return kb;
}

int main(void)
{
    printf("初始    break=%p   [heap]=%5ld KB   匿名段=%5ld KB\n\n",
           sbrk(0), maps_kb(1), maps_kb(0));

    /* A. 连续小块：主堆一台阶一台阶地长 */
    puts("A. malloc(16KB) x 10，都不 free：");
    for (int i = 0; i < 10; i++) {
        char *p = malloc(16 * 1024);
        if (p == NULL) { perror("malloc"); return 1; }
        memset(p, 'a', 16 * 1024);
        printf("   #%-2d   break=%p   [heap]=%5ld KB   匿名段=%5ld KB\n",
               i + 1, sbrk(0), maps_kb(1), maps_kb(0));
    }

    /* B. 大块：每次都在别处新开一段匿名映射，主堆纹丝不动 */
    puts("\nB. malloc(256KB) x 4，都不 free：");
    for (int i = 0; i < 4; i++) {
        char *p = malloc(256 * 1024);
        if (p == NULL) { perror("malloc"); return 1; }
        memset(p, 'b', 256 * 1024);
        printf("   #%-2d   break=%p   [heap]=%5ld KB   匿名段=%5ld KB\n",
               i + 1, sbrk(0), maps_kb(1), maps_kb(0));
    }
    return 0;
}
