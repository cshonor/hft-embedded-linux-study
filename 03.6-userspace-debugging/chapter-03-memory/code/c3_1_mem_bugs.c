/*
 * c3_1_mem_bugs.c —— 五个内存错误，一个程序，用 argv 选
 *
 * 用途：3.1（valgrind memcheck）与 3.2（ASan）共用同一份「病人」。
 *       同一份源码在两套工具下的报告可以逐字段对照，这是本模块
 *       「一个 bug、多种工具」讲法的基础。
 *
 * 用法：./c3_1_mem_bugs <1..5>
 *         1 = 堆越界写（heap-buffer-overflow）
 *         2 = 栈越界写（stack-buffer-overflow）★ valgrind 抓不到，只有 ASan 能抓
 *         3 = use-after-free（写已 free 的堆块）
 *         4 = double free
 *         5 = 内存泄漏（只 malloc，从不 free）
 *
 * 编译（默认）：
 *   gcc -g -O0 -Wall -Wextra -o c3_1_mem_bugs c3_1_mem_bugs.c
 * 编译（ASan，注意是 -O0）：
 *   gcc -g -O0 -fsanitize=address -o c3_1_mem_bugs_asan c3_1_mem_bugs.c
 *
 * ⚠️ 为什么这套 demo 用 -O0 而不是 ASan 官方推荐的 -O1（以下是实测结论，不是推测）：
 *   gcc 13.3 实测，-O1 -fsanitize=address 下五个 case 的检出情况：
 *     case 1 堆越界   → 仍检出（heap-buffer-overflow）
 *     case 2 栈越界   → 仍检出，但 bug_stack_overflow 被内联进 main，
 *                     栈帧名变成 main、写方变成 memset（形参/结构都被优化改写）
 *     case 3 UAF      → ★ 检不出！exit 0、零报告。p[0] = 7 与随后的 g_sink = p[0]
 *                     一起成了死代码被删掉，UAF 这件事在 -O1 下根本不存在了
 *     case 4 double free → 仍检出（free() 调用不可能被消除）
 *     case 5 泄漏     → 仍检出（malloc 结果必须产生，否则 free 无对象）
 *   所以 -O0 是「让五个 case 全部现形」的前提；同时它也是一条重要教训：
 *   **优化会改变 bug 的存在性**（不只是改变报告措辞）——release 用 -O2 时，
 *   一批「写完就死」的内存错误会集体隐身。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static volatile int g_sink;         /* 让「读」这件事真发生，别被优化掉 */

/* ---- case 1：堆越界写 ---- */
static void bug_heap_overflow(void)
{
    char *buf = malloc(8);          /* 申请 8 字节 */
    if (!buf)
        return;
    memset(buf, 'A', 8);
    buf[8] = 'X';                   /* ← 第 9 个字节：踩进 malloc 块右侧的红区 */
    g_sink = buf[0];
    printf("case 1: 写了 buf[8]（块只有 8 字节），程序继续跑\n");
    free(buf);
}

/* ---- case 2：栈越界写 ---- */
static void bug_stack_overflow(void)
{
    char src[32];
    char dst[8];

    memset(src, 'B', sizeof(src));
    memcpy(dst, src, sizeof(src));  /* ← 32 字节塞进 8 字节的栈数组 */
    g_sink = dst[0];
    printf("case 2: 把 32 字节 memcpy 进 char dst[8]，程序继续跑\n");
}

/* ---- case 3：use-after-free ---- */
static void bug_use_after_free(void)
{
    int *p = malloc(4 * sizeof(int));
    if (!p)
        return;
    p[0] = 42;
    free(p);
    p[0] = 7;                       /* ← 释放后再写：踩进已中毒的隔离区 */
    g_sink = p[0];
    printf("case 3: free 后又写了 p[0]，程序继续跑\n");
}

/* ---- case 4：double free ---- */
static void bug_double_free(void)
{
    int *p = malloc(16);
    if (!p)
        return;
    free(p);
    free(p);                        /* ← 同一指针释放两次 */
    printf("case 4: 同一指针 free 了两次，程序继续跑\n");
}

/* ---- case 5：内存泄漏 ---- */
static void bug_leak(void)
{
    for (int i = 0; i < 100; i++) {
        char *snapshot = malloc(400);   /* 订单快照：借了 100 次，一次没还 */
        if (!snapshot)
            return;
        memset(snapshot, 0, 400);
    }
    printf("case 5: 分配了 100 × 400 = 40000 字节，全部未释放\n");
}

int main(int argc, char **argv)
{
    int which = argc > 1 ? atoi(argv[1]) : 1;

    switch (which) {
    case 1: bug_heap_overflow();   break;
    case 2: bug_stack_overflow();  break;
    case 3: bug_use_after_free();  break;
    case 4: bug_double_free();     break;
    case 5: bug_leak();            break;
    default:
        fprintf(stderr, "用法: %s <1..5>\n", argv[0]);
        return 2;
    }

    printf("case %d 跑完了 main（没有崩，也没有任何提示 —— 这正是内存 bug 阴险的地方）\n",
           which);
    return 0;
}
