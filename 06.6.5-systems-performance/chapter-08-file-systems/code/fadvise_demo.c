/* fadvise_demo.c — posix_fadvise 两个最常用提示（ch08 §8.1-8.3 块1）
 *
 *   POSIX_FADV_SEQUENTIAL  提示顺序读 → 内核加大预读窗口
 *   POSIX_FADV_DONTNEED    提示可丢弃 cache → 大文件扫一遍不挤占页缓存
 *
 * HFT 场景：回放历史行情文件时，DONTNEED 防止几个 GB 的回放把
 *           热数据的页缓存挤掉（回放完缓存里全是垃圾）。
 *
 * 编译: gcc -g -O2 -Wall -o fadvise_demo fadvise_demo.c
 * 运行: ./fadvise_demo <大文件路径>   （读一遍并报告耗时/页缓存变化）
 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static double now_s(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "用法: %s <文件路径>\n", argv[0]);
        return 1;
    }
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) { perror("open"); return 1; }

    /* ① 提示顺序读：内核把预读窗口放大（默认 128KB → 更多） */
    if (posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL) != 0)
        perror("fadvise SEQUENTIAL");

    char *buf = malloc(1 << 20);           /* 1MB 缓冲 */
    ssize_t n;
    long long total = 0;
    double t0 = now_s();
    while ((n = read(fd, buf, 1 << 20)) > 0)
        total += n;
    double dt = now_s() - t0;
    printf("顺序读 %lld 字节，%.3fs → %.1f MB/s\n", total, dt, total / dt / 1e6);

    /* ② 提示可丢弃：这文件的数据不需要留在页缓存里 */
    if (posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED) != 0)
        perror("fadvise DONTNEED");
    printf("已对整文件发 DONTNEED：对应页缓存可被内核回收（脏页先写回）\n");

    free(buf);
    close(fd);
    return 0;
}
