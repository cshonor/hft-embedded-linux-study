/*
 * demo/ptrbug.c —— 给**任何**调试器当靶子的小程序
 *
 * 它干什么：一个"看起来完全正确、实际越界读"的指针循环。
 * 用 VS 的图形调试器、Xcode 的图形调试器、或 lldb/gdb 命令行，
 * 在 sum_n() 的循环里下一次断点，你会亲眼看见 i 超过数组长度之后，
 * a[i] 在读的是谁的内存。
 *
 * 为什么要这么写：
 *  - 指针 + 数组 + 长度分离，是 C 里最常见的崩法（对应 01-c-language 主线）
 *  - 越界**读**通常不崩，只是得到一个随机值 —— 这比崩溃更难查，
 *    也正是调试器比 printf 强的地方：不用改代码就能看见每一帧
 *
 * 三个工具都编译它：
 *   clang -g -O0 -Wall -Wextra -std=c11 ptrbug.c -o ptrbug   # macOS / Linux
 *   gcc   -g -O0 -Wall -Wextra -std=c11 ptrbug.c -o ptrbug   # Linux / Pi
 *   cl /Od /Zi /W4 /std:c17 ptrbug.c                         # Windows MSVC
 */

#include <stdio.h>

#define N 4

static int buf[N] = { 0, 0, 0, 0 };

/*
 * n 由调用者给出，函数内部无法验证它是否 <= 数组真实长度。
 * 这就是 C 的经典问题：数组传进函数就退化成指针，长度信息丢失了。
 */
static int sum_n(const int *a, int n)
{
    int s = 0;
    for (int i = 0; i < n; i++) {
        s += a[i];              /* ← 建议在这行下断点，单步看 i 和 a+i */
    }
    return s;
}

int main(void)
{
    for (int i = 0; i < N; i++) {
        buf[i] = (i + 1) * 10;  /* 10 20 30 40 */
    }

    printf("sizeof(buf)      = %zu bytes, %zu elems\n",
           sizeof buf, sizeof buf / sizeof buf[0]);

    int ok  = sum_n(buf, N);    /* 正确用法 */
    int bad = sum_n(buf, 8);    /* 故意越界读：多读 4 个 int */

    printf("sum_n(buf, %d)   = %d\n", N, ok);
    printf("sum_n(buf, 8)    = %d   <-- 越界读，值不确定但通常不崩\n", bad);

    int *p = buf;
    printf("buf=%p  buf[0]=%d  *(p+3)=%d\n",
           (void *)buf, buf[0], *(p + 3));

    return 0;
}
