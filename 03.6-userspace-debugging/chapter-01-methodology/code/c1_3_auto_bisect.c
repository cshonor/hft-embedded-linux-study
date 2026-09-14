/*
 * c1_3_auto_bisect.c —— 把「数据二分」写成程序：自动收敛到那条坏记录
 *
 * 用途：1.3 讲「每次砍半」。人肉二分会累、会抄错区间，写成程序就变成
 *       一条可复读、可验证的日志 —— 16 条记录只需 4 次判定。
 *
 * 三个可直接观察的点：
 *   1. **整数除零在 Linux 上发的是 SIGFPE**（名字叫「浮点异常」，其实整数除零也走它）；
 *   2. **抓现场靠 sigsetjmp + siglongjmp**：把「这一批崩了」变成一个可返回的布尔值，
 *      于是二分循环能一直跑下去，不用 fork 子进程；
 *   3. **判定函数必须是纯布尔**（bad/good）—— 这正是二分能成立的前提（见 1.3 末节）。
 *
 * 编译：gcc -g -O0 -Wall -Wextra -o c1_3_auto_bisect c1_3_auto_bisect.c
 * 运行：./c1_3_auto_bisect
 */
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>

#define N 16
#define CULPRIT 11                  /* 下标 11（即「第 12 条」）的 volume 是 0：人先知道，程序不知道 */

static sigjmp_buf g_jmp;
static volatile int g_caught;

static void on_sigfpe(int sig)
{
    (void)sig;
    g_caught = 1;
    siglongjmp(g_jmp, 1);           /* 从信号处理函数里跳回判定函数 */
}

/* 每条记录的 volume：只有 CULPRIT 那条是 0，其余都是 100 */
static long volume_of(int idx)
{
    return (idx == CULPRIT) ? 0 : 100;
}

static long turnover_of(int idx)
{
    return 1000 + idx;
}

/*
 * 处理 records [lo, hi]，返回 1 = 全批正常，0 = 这一批里有记录能把程序炸掉。
 * 注意：真正的业务代码不会这么写 —— 这里把「崩了」当成可捕获的返回值，
 * 只是为了给二分提供一个自动判定（生产上对应的是「跑一遍测试脚本看退出码」）。
 */
static int batch_ok(int lo, int hi)
{
    if (sigsetjmp(g_jmp, 1) != 0)
        return 0;                   /* 从 SIGFPE 跳回来 = 这一批坏 */

    g_caught = 0;
    for (int i = lo; i <= hi; i++) {
        volatile long vol = volume_of(i);
        volatile long turn = turnover_of(i);
        volatile long vwap = turn / vol;        /* ← i == CULPRIT 时除零 */
        (void)vwap;
    }
    return 1;
}

int main(void)
{
    struct sigaction sa = {0};

    sa.sa_handler = on_sigfpe;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGFPE, &sa, NULL);

    printf("共 %d 条记录，先确认「整批是坏的」\n", N);
    printf("  判定 [0, %d] -> %s\n", N - 1, batch_ok(0, N - 1) ? "good" : "BAD");
    printf("开始二分（每次砍半）：\n");

    int lo = 0, hi = N - 1, step = 0;
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        step++;
        if (batch_ok(lo, mid)) {
            printf("  第 %d 步: [%2d, %2d] good  -> 嫌疑落在右半 [%2d, %2d]\n",
                   step, lo, mid, mid + 1, hi);
            lo = mid + 1;
        } else {
            printf("  第 %d 步: [%2d, %2d] BAD   -> 嫌疑落在左半 [%2d, %2d]\n",
                   step, lo, mid, lo, mid);
            hi = mid;
        }
    }

    printf("收敛：坏记录是第 %d 条（下标 %d）—— %d 条记录共 %d 步，因为 log2(%d) = %d\n",
           lo + 1, lo, N, step, N, step);
    printf("该记录 volume = %ld，turnover = %ld —— 除零的根因就在这里\n",
           volume_of(lo), turnover_of(lo));
    return 0;
}
