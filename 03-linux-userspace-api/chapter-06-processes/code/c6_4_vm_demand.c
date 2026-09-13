/* c6_4_vm_demand.c — 6.4 虚拟内存：按需分页 / 零页 / RSS 只统计真碰过的页
 * 编译: gcc -O2 -Wall -Wextra -o c6_4_vm_demand c6_4_vm_demand.c
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define MB (1024UL * 1024)

/* 用 /proc/self/statm 第 2 列拿常驻集（单位：页） */
static long rss_pages(void)
{
    int fd = open("/proc/self/statm", O_RDONLY);
    if (fd < 0) return -1;
    char b[128];
    ssize_t n = read(fd, b, sizeof b - 1);
    close(fd);
    if (n <= 0) return -1;
    b[n] = 0;
    long size = 0, resident = 0;
    sscanf(b, "%ld %ld", &size, &resident);
    return resident;
}

static void report(const char *tag, long base, long ps)
{
    long r = rss_pages();
    printf("  %-26s RSS = %6.1f MB  (%+.1f MB)\n",
           tag, (double)r * ps / MB, (double)(r - base) * ps / MB);
}

int main(void)
{
    long ps = sysconf(_SC_PAGESIZE);
    printf("页大小 = %ld B\n", ps);

    /* 1. malloc 128MB：只是登记 VMA，一个物理页都没给 */
    size_t sz = 128 * MB;
    char *p = malloc(sz);
    if (!p) { perror("malloc"); return 1; }
    long base = rss_pages();
    report("malloc(128MB) 后", base, ps);

    /* 2. 只读一遍：读缺页挂全局零页，RSS 纹丝不动 */
    long sum = 0;
    for (size_t i = 0; i < sz; i += (size_t)ps)
        sum += p[i];
    report("全量读一遍后", base, ps);

    /* 3. 写一半：首写才分配清零实页 */
    memset(p, 0x5A, sz / 2);
    report("写前一半后", base, ps);

    printf("  (读到的字节和 = %ld，证明页确实被访问过)\n", sum);
    free(p);
    return 0;
}
