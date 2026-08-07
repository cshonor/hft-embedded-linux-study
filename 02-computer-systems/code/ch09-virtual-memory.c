/*
 * CSAPP Ch9 · 虚拟内存 — mmap + 页对齐分配 + mprotect
 *
 * 对照笔记:
 *   chapter-09/notes/section-9.8-内存映射mmap.md
 *   chapter-09/notes/section-9.6-地址翻译.md
 *
 * 编译:
 *   gcc -Wall -Wextra -std=c11 -O2 -o ch09_vm ch09-virtual-memory.c
 * 运行:
 *   ./ch09_vm
 *
 * HFT 关联: 大页(huge pages)、页对齐分配、零拷贝 mmap 文件
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

/* ---------- 页大小 ---------- */
static long page_size = 0;

static void print_page_info(void)
{
    page_size = sysconf(_SC_PAGESIZE);
    printf("=== 系统页信息 ===\n\n");
    printf("  PAGE_SIZE = %ld bytes (%ld KB)\n", page_size, page_size / 1024);
#ifdef __linux__
    printf("  huge page = %d bytes (2 MB)\n", 2 * 1024 * 1024);
    printf("  /proc/sys/vm/nr_hugepages 查看可用大页数\n");
#endif
    printf("\n");
}

/* ---------- 1. 页对齐分配 (malloc 不保证页对齐) ---------- */
static void demo_aligned_alloc(void)
{
    printf("=== 1. 页对齐分配 ===\n\n");

    /* 方法 A: posix_memalign */
    void *ptr1 = NULL;
    if (posix_memalign(&ptr1, page_size, page_size * 4) == 0) {
        printf("  posix_memalign:  ptr=%p  (对齐 %ldB)\n", ptr1, page_size);
        free(ptr1);
    }

    /* 方法 B: aligned_alloc (C11) */
    void *ptr2 = aligned_alloc(page_size, page_size * 4);
    if (ptr2) {
        printf("  aligned_alloc:   ptr=%p  (对齐 %ldB)\n", ptr2, page_size);
        free(ptr2);
    }

    /* 方法 C: mmap (匿名映射, 自动页对齐) */
    void *ptr3 = mmap(NULL, page_size * 4, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr3 != MAP_FAILED) {
        printf("  mmap anonymous:  ptr=%p  (自动页对齐)\n", ptr3);
        munmap(ptr3, page_size * 4);
    }

    /* HFT: 为什么需要页对齐? */
    printf("\n  HFT: 订单簿/环形缓冲区页对齐 → 减少 TLB miss\n");
    printf("       大页 (2MB) → 减少 TLB 条目数, 减少 miss\n\n");
}

/* ---------- 2. mmap 文件映射 (零拷贝读取) ---------- */
static void demo_mmap_file(void)
{
    printf("=== 2. mmap 文件映射 ===\n\n");

    /* 创建临时文件 */
    const char *tmpfile = "/tmp/ch09_mmap_demo.txt";
    int fd = open(tmpfile, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { perror("open"); return; }

    const char *msg = "Hello from mmap! This is memory-mapped file content.\n";
    write(fd, msg, strlen(msg));

    /* mmap 映射到内存 */
    struct stat st;
    fstat(fd, &st);
    char *mapped = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) { perror("mmap"); close(fd); return; }

    printf("  文件大小: %ld bytes\n", (long)st.st_size);
    printf("  映射地址: %p\n", (void*)mapped);
    printf("  内容: %s", mapped);

    /* 修改映射内容 (需要 MAP_SHARED + PROT_WRITE) */
    munmap(mapped, st.st_size);
    close(fd);
    unlink(tmpfile);

    printf("\n  HFT: 行情数据文件 mmap → 零拷贝读取, 无需 read() 系统调用\n\n");
}

/* ---------- 3. mprotect 内存保护 ---------- */
static void demo_mprotect(void)
{
    printf("=== 3. mprotect 内存保护 ===\n\n");

    /* 分配一页, 初始可读写 */
    char *page = mmap(NULL, page_size, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (page == MAP_FAILED) { perror("mmap"); return; }

    strcpy(page, "可读写区域");
    printf("  初始: \"%s\" (PROT_READ|PROT_WRITE)\n", page);

    /* 改为只读 */
    if (mprotect(page, page_size, PROT_READ) == 0) {
        printf("  mprotect → PROT_READ only\n");
        printf("  读取: \"%s\" ✓\n", page);
        /* 写入会 SIGSEGV — 解注释验证:
         * page[0] = 'X';  // → Segmentation fault
         */
        printf("  写入: 会触发 SIGSEGV (已注释, 解注释验证)\n");
    }

    munmap(page, page_size);

    printf("\n  HFT: 关键数据结构设只读 → 防止意外写入\n\n");
}

/* ---------- 4. 大页 (Huge Pages, Linux) ---------- */
#ifdef __linux__
static void demo_hugepage(void)
{
    printf("=== 4. 大页 (Huge TLB Pages) ===\n\n");

    size_t huge_size = 2 * 1024 * 1024;  /* 2MB */
    void *huge = mmap(NULL, huge_size, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    if (huge != MAP_FAILED) {
        printf("  mmap MAP_HUGETLB: ptr=%p  size=2MB\n", huge);
        /* 写入测试 */
        memset(huge, 0, huge_size);
        printf("  写入 2MB ✓ (TLB 只需 1 个条目 vs 普通 512 个)\n");
        munmap(huge, huge_size);
    } else {
        printf("  MAP_HUGETLB 失败: %s\n", strerror(errno));
        printf("  (需要先设置: echo 20 > /proc/sys/vm/nr_hugepages)\n");
    }

    printf("\n  HFT: 大页减少 TLB miss → 稳定低延迟\n");
    printf("       普通页: 2MB 需 512 个 TLB 条目\n");
    printf("       大页:   2MB 需 1 个 TLB 条目\n\n");
}
#endif

int main(void)
{
    print_page_info();

    demo_aligned_alloc();
    demo_mmap_file();
    demo_mprotect();
#ifdef __linux__
    demo_hugepage();
#endif

    printf("关键点:\n");
    printf("  1. mmap 匿名映射 → 页对齐内存, 无需 malloc\n");
    printf("  2. mmap 文件映射 → 零拷贝读取\n");
    printf("  3. mprotect → 运行时内存保护\n");
    printf("  4. MAP_HUGETLB → 大页减少 TLB pressure\n");

    return 0;
}
