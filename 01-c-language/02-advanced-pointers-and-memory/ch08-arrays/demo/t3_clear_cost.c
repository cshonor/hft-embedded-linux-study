/* t3_clear_cost.c —— 清零不是免费的：HFT 里什么时候该清、什么时候是纯浪费
 *
 * 核心问题：「HFT 里是不是统一都要 = {0}？」
 * 答案：不是「统一要」，而是「分清两类缓冲」——
 *   - 需要确定性初值的（输出参数、状态机、握手前）→ 必须清零
 *   - 马上被完整覆盖的（read/recv 之后按返回值用）→ 清零是纯开销
 *
 * WSL: gcc -std=c11 -O2 -Wall -o t3 t3_clear_cost.c && ./t3
 */
#define _POSIX_C_SOURCE 200809L   /* 暴露 clock_gettime / CLOCK_MONOTONIC */
#include <stdio.h>
#include <string.h>
#include <time.h>

#define N   4096
#define REP 200000

static volatile int sink;

static double now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

/* 场景 A：清零后「完整」覆盖 —— 清零是死存储 */
__attribute__((noinline))
static void case_full_overwrite(void)
{
    char buf[N] = {0};                 /* 清零…… */
    memset(buf, 'A', sizeof buf);      /* ……立刻被这行完全覆盖 */
    sink = buf[N - 1];
}

/* 场景 B：只覆盖前 k 字节，后面靠 0 兜底 —— 清零不可省 */
__attribute__((noinline))
static void case_partial_use(int k)
{
    char buf[N] = {0};                 /* 必须清：后面按「0 结尾」语义用 */
    memset(buf, 'B', (size_t)k);
    sink = buf[N - 1];
}

/* 场景 C：输出参数语义 —— 清零是正确性要求，不是性能选择 */
__attribute__((noinline))
static void case_out_param(int *out, int n)
{
    memset(out, 0, (size_t)n * sizeof *out);   /* 调不好就返回脏值 */
    out[1] = 7;                                /* 只填了一部分 */
}

int main(void)
{
    double t0, t1;

    printf("======== 1. 场景 A：清零 + 立即完整覆盖（DSE 机会）========\n");
    t0 = now_ms();
    for (int i = 0; i < REP; i++) case_full_overwrite();
    t1 = now_ms();
    printf("  %d 次 × %d 字节: %8.2f ms\n", REP, N, t1 - t0);
    printf("  → -O2 下编译器应把 `= {0}` 的 memset 消除（见 run.sh 汇编对比）\n\n");

    printf("======== 2. 场景 B：只有前 k 字节被覆盖（清零不可省）========\n");
    t0 = now_ms();
    for (int i = 0; i < REP; i++) case_partial_use(16);
    t1 = now_ms();
    printf("  k=16 时 %d 次: %8.2f ms\n", REP, t1 - t0);
    t0 = now_ms();
    for (int i = 0; i < REP; i++) case_partial_use(N);
    t1 = now_ms();
    printf("  k=%d 时 %d 次: %8.2f ms\n", N, REP, t1 - t0);
    printf("  → 覆盖面积不同，但零条指令数一样：一次 memset 固定成本\n\n");

    printf("======== 3. 场景 C：输出参数必须清零 ========\n");
    int out[8];
    case_out_param(out, 8);
    printf("  out = {");
    for (int i = 0; i < 8; i++) printf("%d%s", out[i], i == 7 ? "" : ", ");
    printf("}\n");
    printf("  → out[0]=0 来自清零，out[1]=7 来自赋值；不清零 out[0] 就是脏值\n\n");

    printf("======== 4. 全局/static：.bss 免费清零，不吃指令 ========\n");
    static char big[N];                 /* 落 .bss，加载时内核清零 */
    printf("  static char big[%d]: 首次使用即全 0，热路径零开销\n", N);
    printf("  → 大缓冲优先声明为全局/static，而不是栈上 `= {0}`\n\n");

    printf("======== 5. 结论 ========\n");
    printf("  「= {0}」是标准语义（首元素显式 0 + 其余隐式补 0），结果=全 0；\n");
    printf("  但它不是 HFT 的无条件铁律 —— 判据是「这块内存是否马上被完整覆盖」：\n");
    printf("    需要确定性初值 / 输出参数 / 安全性兜底 → 清零\n");
    printf("    马上被完整写入的纯中转缓冲            → 不清，省一次 memset\n");
    return 0;
}
