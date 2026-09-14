/* TLPI 第 2 章 §2.8 —— 内存映射：MAP_PRIVATE vs MAP_SHARED 的可见性差异
 *
 * 编译：gcc -O0 -Wall -Wextra c2_8_mmap.c -o c2_8
 * 运行：./c2_8
 *
 * 本节要钉死的事实：
 *   ① mmap 把文件/设备映射进用户地址空间，之后读写直接访存，不再有 read/write syscall。
 *   ② MAP_PRIVATE = 写时复制（COW）：改动只在本进程可见，不落回文件。
 *   ③ MAP_SHARED  = 改动直接写进页缓存，对**所有**映射同一文件的进程可见，
 *      也最终会落到文件（msync 只是强制刷盘，不是「让改动可见」的前提）。
 *   ④ MAP_ANONYMOUS 不要文件，就是一块匿名内存（malloc 大块时的底层手段）。
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MMDIR "/tmp/c2_8"

static void write_file(const char *path, const char *content)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { perror("open"); return; }
    if (write(fd, content, strlen(content)) < 0) perror("write");
    close(fd);
}

static void read_file(const char *path, char *buf, size_t sz)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) { snprintf(buf, sz, "(打不开: %s)", strerror(errno)); return; }
    ssize_t n = read(fd, buf, sz - 1);
    close(fd);
    buf[n < 0 ? 0 : n] = '\0';
}

/* 在 /proc/self/maps 里找某个地址属于哪一段 */
static void find_mapping(const void *addr, char *out, size_t osz)
{
    FILE *f = fopen("/proc/self/maps", "r");
    if (!f) { snprintf(out, osz, "(读不到 maps)"); return; }
    char line[512];
    unsigned long lo, hi;
    unsigned long a = (unsigned long)addr;
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%lx-%lx", &lo, &hi) == 2 && a >= lo && a < hi) {
            line[strcspn(line, "\n")] = '\0';
            snprintf(out, osz, "%s", line);
            fclose(f);
            return;
        }
    }
    fclose(f);
    snprintf(out, osz, "(没找到)");
}

int main(void)
{
    char path[128];
    snprintf(path, sizeof(path), "%s/data.txt", MMDIR);
    mkdir(MMDIR, 0755);
    char buf[256], mapinfo[512];

    /* ================= ① MAP_PRIVATE ================= */
    printf("=== ① MAP_PRIVATE：写时复制，改动不落回文件 ===\n");
    write_file(path, "AAAA");
    read_file(path, buf, sizeof(buf));
    printf("  映射前文件内容 = \"%s\"\n", buf);

    int fd = open(path, O_RDWR);
    if (fd < 0) { perror("open"); return 1; }
    struct stat st;
    if (fstat(fd, &st) < 0) { perror("fstat"); return 1; }
    printf("  fstat: st_size=%ld  st_ino=%ld   （st_size 就是要传给 mmap 的长度）\n",
           (long)st.st_size, (long)st.st_ino);

    char *p = mmap(NULL, (size_t)st.st_size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE, fd, 0);
    if (p == MAP_FAILED) { perror("mmap MAP_PRIVATE"); return 1; }
    printf("  mmap(MAP_PRIVATE) 返回地址 = %p\n", (void *)p);
    find_mapping(p, mapinfo, sizeof(mapinfo));
    printf("  /proc/self/maps 里这一段 = %s\n", mapinfo);

    printf("  映射里第 1 个字节 = '%c'\n", p[0]);
    p[0] = 'X';                          /* 只改内存，不调 write */
    printf("  把它改成 'X'（只碰内存，没调 write）\n");
    msync(p, (size_t)st.st_size, MS_SYNC);
    munmap(p, (size_t)st.st_size);

    read_file(path, buf, sizeof(buf));
    printf("  munmap 之后读文件 = \"%s\"\n", buf);
    printf("  -> 文件里还是 AAAA：MAP_PRIVATE 的改动被 COW 拦在了内存里。\n");

    /* ================= ② MAP_SHARED ================= */
    printf("\n=== ② MAP_SHARED：改动直接进页缓存，会落到文件 ===\n");
    write_file(path, "AAAA");
    char *q = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (q == MAP_FAILED) { perror("mmap MAP_SHARED"); return 1; }
    printf("  映射前文件 = \"AAAA\"\n");
    q[0] = 'Y';
    printf("  在映射里把第 1 字节改成 'Y'\n");
    msync(q, 4, MS_SYNC);
    munmap(q, 4);
    read_file(path, buf, sizeof(buf));
    printf("  munmap 之后读文件 = \"%s\"\n", buf);
    printf("  -> 变成 YAAA：MAP_SHARED 的写直接改了页缓存里的那份数据。\n");

    /* ================= ③ 两个 SHARED 映射互相可见 ================= */
    printf("\n=== ③ 同一文件的两个 MAP_SHARED 映射，互相能看见吗？ ===\n");
    write_file(path, "1234");
    char *a = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    char *b = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (a == MAP_FAILED || b == MAP_FAILED) { perror("mmap x2"); return 1; }
    printf("  映射 A 地址=%p  内容=\"%.4s\"\n", (void *)a, a);
    printf("  映射 B 地址=%p  内容=\"%.4s\"\n", (void *)b, b);
    printf("  两个虚拟地址不同，但底层是同一组物理页。\n");
    a[0] = '9';
    printf("  只通过 A 改成 '9'，然后立刻读 B（不调 msync）：\n");
    printf("    A = \"%.4s\"\n", a);
    printf("    B = \"%.4s\"   <- B 也变了，这就是「共享」\n", b);
    printf("  -> 对照 ①：MAP_PRIVATE 时两个映射互不可见。\n");
    munmap(a, 4); munmap(b, 4);
    close(fd);

    /* ================= ④ 匿名映射 ================= */
    printf("\n=== ④ MAP_ANONYMOUS：不要文件的匿名内存 ===\n");
    size_t len = 4096;
    char *anon = mmap(NULL, len, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (anon == MAP_FAILED) { perror("mmap ANONYMOUS"); return 1; }
    printf("  mmap(NULL, %zu, ..., MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = %p\n",
           len, (void *)anon);
    find_mapping(anon, mapinfo, sizeof(mapinfo));
    printf("  maps 里这一段 = %s\n", mapinfo);

    strcpy(anon, "anonymous page");
    printf("  往里写字符串再读回：\"%s\"\n", anon);

    if (mprotect(anon, len, PROT_READ) == 0)
        printf("  mprotect(只读) 成功：再往这里写就会 SIGSEGV（本 demo 不真写）\n");
    munmap(anon, len);
    find_mapping(anon, mapinfo, sizeof(mapinfo));
    printf("  munmap 之后再查 maps = %s\n", mapinfo);

    printf("\n=== ⑤ 一张对照表 ===\n");
    printf("  %-26s %-28s %s\n", "flag", "visible to others?", "written back?");
    printf("  %-26s %-28s %s\n", "--------------------------",
           "----------------------------", "----------------------------");
    printf("  %-26s %-28s %s\n", "MAP_PRIVATE", "no  (copy-on-write)", "no");
    printf("  %-26s %-28s %s\n", "MAP_SHARED", "yes (same physical pages)",
           "yes (msync only flushes)");
    printf("  %-26s %-28s %s\n", "MAP_PRIVATE|MAP_ANONYMOUS",
           "no other process at all", "no file at all");
    printf("\n  怎么选：\n");
    printf("    · 改了别人要看得见、还要落盘   -> MAP_SHARED\n");
    printf("    · 随便改，千万别动文件         -> MAP_PRIVATE\n");
    printf("    · 只是要一块内存               -> MAP_PRIVATE|MAP_ANONYMOUS\n");
    return 0;
}
