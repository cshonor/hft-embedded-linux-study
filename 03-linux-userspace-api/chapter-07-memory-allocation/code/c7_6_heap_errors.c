/* Ch7 §7.1 — 五类典型堆错误，用 ASan 抓现行
 * 每次只编一种（CASE 由编译期决定），这样每个 case 都在干净的进程里跑：
 *
 *   for n in 1 2 3 4 5; do
 *       gcc -O1 -g -fsanitize=address -DCASE=$n -Wall -Wextra \
 *           -o c7_6_case$n c7_6_heap_errors.c
 *       ./c7_6_case$n
 *   done
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __SANITIZE_ADDRESS__
#include <sanitizer/lsan_interface.h>
#endif

#ifndef CASE
#define CASE 1
#endif

/* 1. 堆越界写：踩到分配器元数据 / ASan 的 redzone */
static void case_oob(void)
{
    char *raw = malloc(8);
    volatile char *p = raw;
    p[16] = 'x';                        /* 越界 8 字节，落在 ASan 的 redzone 里 */
    free(raw);
}

/* 2. use-after-free */
static void case_uaf(void)
{
    char *p = malloc(32);
    free(p);
    printf("     (uaf) 释放后读到的字节 = %d\n", p[0]);
}

/* 3. double free */
static void case_double_free(void)
{
    char *p = malloc(32);
    printf("     (df) 第一块 %p\n", (void *) p);
    free(p);
    free(p);
}

/* 4. 内存泄漏：
 *    大多数 ASan 环境在进程退出时由 LSan 汇报泄漏，但有些容器里
detect_leaks 默认是关的（实测 Compiler Explorer 就是）。
 *    所以这里换成不依赖工具的观测法：看 RSS 涨不涨。 */
static long rss_kb(void)
{
    FILE *f = fopen("/proc/self/statm", "r");
    if (f == NULL) return -1;
    long total = 0, resident = 0;
    if (fscanf(f, "%ld %ld", &total, &resident) != 2) resident = -1;
    fclose(f);
    return resident * (sysconf(_SC_PAGESIZE) / 1024);
}

static void case_leak(void)
{
    long before = rss_kb();
    for (int i = 0; i < 1000; i++) {
        void *p = malloc(4096);         /* 指针当场丢失 */
        if (p != NULL) memset(p, 1, 4096);  /* 真碰，否则物理页不会到手 */
    }
    printf("     (leak) 1000 x 4KB 分配后不释放：RSS %+ld KB\n",
           rss_kb() - before);
    fflush(stdout);                     /* LSan 报错后直接 _exit，缓冲会丢 */
#ifdef __SANITIZE_ADDRESS__
    __lsan_do_leak_check();             /* 环境若开了 LSan，这里会额外报一份 */
#endif
}

/* 5. free 一个非堆指针（这里是栈地址） */
static void case_invalid(void)
{
    int on_stack = 42;
    free(&on_stack);
}

static const struct { const char *name; void (*fn)(void); } CASES[] = {
    { "(无)",                            NULL },
    { "越界写     heap-buffer-overflow",  case_oob },
    { "释放后使用 heap-use-after-free",   case_uaf },
    { "重复释放   double-free",           case_double_free },
    { "内存泄漏   leak",                  case_leak },
    { "free 非堆  bad-free",              case_invalid },
};

int main(void)
{
    printf("CASE %d: %s\n", CASE, CASES[CASE].name);
    fflush(stdout);
    CASES[CASE].fn();
    return 0;
}
