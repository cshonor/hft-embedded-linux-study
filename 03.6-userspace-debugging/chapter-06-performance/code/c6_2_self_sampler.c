/*
 * c6_2_self_sampler.c —— 不装 perf，也能量出「CPU 花在哪个函数上」
 *
 * 6.1 讲了 perf record 的原理就三句话：
 *   ① 内核/硬件定期发中断（默认按 CPU cycles）
 *   ② 中断处理程序记录当时的 PC + 调用栈
 *   ③ 事后按函数聚合，占比高的就是热点
 *
 * 这个程序把这三步在**进程内**自己实现一遍，只用标准库就够：
 *   ① setitimer(ITIMER_PROF) —— 按「进程 CPU 时间」计时，到点发 SIGPROF
 *      （注意是 CPU 时间不是墙钟时间：进程被调度走时不计时，这正是 profiler 要的）
 *   ② SIGPROF 处理函数里 backtrace() —— 抓当前调用栈，只存裸 PC
 *      （不做符号解析，因为信号处理函数里不能 malloc）
 *   ③ 跑完后回到普通上下文，用 backtrace_symbols() 解析符号并排出三张表
 *
 * 于是我们能在**任何** Linux 上得到与 perf 同构的结果 ——
 * 包括没有 PMU 权限、连 perf 都没装的容器里。区别只在采样源：
 *   perf 用硬件计数器 + 内核 NMI，我们用的是 setitimer 送来的 SIGPROF。
 *
 * 用法：./c6_2_self_sampler
 * 编译（四个参数缺一不可，理由写在下面）：
 *   cc -g -O2 -fno-inline -rdynamic -Wall -Wextra \
 *      -o c6_2_self_sampler c6_2_self_sampler.c
 *     -g          : 符号表里带源码行信息
 *     -fno-inline : 防止热点函数被内联进调用者 —— 被内联了就采不到独立符号，
 *                   表里只会剩一个大坨的 do_work（这正是 6.1「fno-inline 的取舍」）
 *     -rdynamic   : 把符号导出到 .dynsym，backtrace_symbols 才能给出函数名
 *                   （见 2.3 的 .dynsym vs .symtab 那一节）
 *     ★ 反过来说：把上面的函数改成 static，就没法导出到 .dynsym 了，
 *       表里会变成一堆 "output.s(+0x401914)" 这样的裸偏移 —— 这个现象
 *       Ch2 的 c2_2_backtrace.c 已经踩过一次，这里能再复现一次。
 */

#define _GNU_SOURCE

#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define MAX_SAMPLES 60000
#define MAX_FRAMES  16
#define MAX_SYM     256
#define MAX_STACK   256

/* 一帧里哪些是「采样器自己的噪声」，要跳过。
 * 在 glibc/x86-64 上，从 SIGPROF 处理函数里调 backtrace()，栈是这样：
 *   frame 0 = backtrace() 内部帧
 *   frame 1 = on_prof —— 也就是我们的采样函数自己（它是 static，不进 .dynsym，
 *             所以在表里只会显示成 "+0x45330" 这种裸偏移，永远占 100%）
 *   frame 2 = 真正被打断的那个函数  ← 这才是 "self" 该记的
 *   frame 3+ = 它的各级调用者
 * 所以跳过前 2 帧，self 取跳完之后的第一帧。
 * 换 libc/架构时，如果 inclusive 表第一行出现一个 100% 的怪名字，那就是
 * handler 没被跳干净 —— 把 SKIP_FRAMES 调大 1 即可。 */
#define SKIP_FRAMES 2

/* ---------------- ② 采样落点：只存裸 PC，不在信号处理函数里做任何分配 ------- */
static void *g_pc[MAX_SAMPLES][MAX_FRAMES];
static int   g_depth[MAX_SAMPLES];
static volatile sig_atomic_t g_n       = 0;
static volatile sig_atomic_t g_dropped = 0;

static void on_prof(int sig)
{
    (void)sig;
    int n = (int)g_n;
    if (n >= MAX_SAMPLES) { g_dropped = 1; return; }
    g_depth[n] = backtrace(g_pc[n], MAX_FRAMES);
    g_n = n + 1;
}

/* ---------------- 被观测的「业务代码」：埋两个热点 ----------------
 * ⚠️ 刻意不加 static：加了就只有裸偏移，没有函数名（见文件头说明） */
double slow_sqrt(double x)
{
    double r = x;
    for (int i = 0; i < 25; i++)
        r = (r + x / r) / 2.0;      /* 25 次浮点除法 —— 除法是几十周期的指令 */
    return r;
}

void fmt(double v, char *buf, int cap)
{
    int iv = (int)v;
    for (int i = 0; i < cap - 1; i++)
        buf[i] = (char)('0' + (iv % 10));   /* 10 次整数取模 —— 整数除法同样贵 */
    buf[cap - 1] = 0;
}

void do_work(long rounds)
{
    double sum = 0;
    char   buf[11];
    for (long i = 0; i < rounds; i++) {
        sum += slow_sqrt((double)(i % 100) + 1.0);
        fmt(sum, buf, (int)sizeof buf);
    }
    printf("workload 跑完：sum=%.2f buf=%s\n", sum, buf);
}

/* ---------------- ③ 聚合 ---------------- */

struct symcnt { char name[160]; int self; int incl; };
static struct symcnt g_tab[MAX_SYM];
static int g_ntab = 0;

static struct symcnt *slot_of(const char *nm)
{
    for (int i = 0; i < g_ntab; i++)
        if (strcmp(g_tab[i].name, nm) == 0) return &g_tab[i];
    if (g_ntab >= MAX_SYM) return &g_tab[0];
    snprintf(g_tab[g_ntab].name, sizeof g_tab[0].name, "%s", nm);
    return &g_tab[g_ntab++];
}

/* 把 backtrace_symbols 给的 "./prog(func+0x12) [0x5555...]" 压成 "func"；
 * 没有函数名的（static / 未导出）压成 "+0x12"，保留地址差异不合并。 */
static void short_name(const char *raw, char *out, size_t cap)
{
    const char *l = strchr(raw, '(');
    const char *r = l ? strrchr(l, ')') : NULL;
    if (l && r && r > l) {
        size_t len = (size_t)(r - l - 1);
        if (len > 0) {
            if (len >= cap) len = cap - 1;
            memcpy(out, l + 1, len);
            out[len] = 0;
            char *plus = strchr(out, '+');
            if (plus && plus != out) *plus = 0;   /* 有名字 → 砍掉 +0x 偏移 */
            return;                               /* plus==out → 没名字，保留 "+0x..." */
        }
    }
    snprintf(out, cap, "%s", raw);
}

static int cmp_by_self(const void *a, const void *b)
{
    return (*(const struct symcnt *const *)b)->self - (*(const struct symcnt *const *)a)->self;
}
static int cmp_by_incl(const void *a, const void *b)
{
    return (*(const struct symcnt *const *)b)->incl - (*(const struct symcnt *const *)a)->incl;
}

static void print_sym_table(const char *title, const char *note,
                            const struct symcnt **arr, int n, int total, int which)
{
    printf("%s\n%s\n", title, note);
    printf("  占比      样本  函数\n");
    int printed = 0;
    for (int i = 0; i < n; i++) {
        int c = which ? arr[i]->incl : arr[i]->self;
        if (!which && c == 0) continue;
        printf("  %6.1f%%   %5d  %s\n", 100.0 * c / total, c, arr[i]->name);
        printed++;
    }
    if (printed == 0) printf("  （空）\n");
    printf("\n");
}

/* 折叠栈：就是 stackcollapse-perf.pl 的输出格式 */
struct stackcnt { char key[640]; int n; };
static struct stackcnt g_stk[MAX_STACK];
static int g_nstk = 0;

static void stack_add(const char *key)
{
    for (int i = 0; i < g_nstk; i++)
        if (strcmp(g_stk[i].key, key) == 0) { g_stk[i].n++; return; }
    if (g_nstk >= MAX_STACK) return;
    snprintf(g_stk[g_nstk].key, sizeof g_stk[0].key, "%s", key);
    g_stk[g_nstk].n = 1;
    g_nstk++;
}

static int cmp_stack(const void *a, const void *b)
{
    return ((const struct stackcnt *)b)->n - ((const struct stackcnt *)a)->n;
}

int main(void)
{
    /* 热身一次：backtrace 首次调用要初始化 dl 缓存（可能 malloc），
     * 必须放在装定时器**之前**——信号处理函数里不能 malloc。 */
    { void *w[4]; backtrace(w, 4); }

    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_prof;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;    /* 默认：处理期间自动屏蔽 SIGPROF，不会嵌套进来 */
    sigaction(SIGPROF, &sa, NULL);

    /* ① 每 1ms CPU 时间一次；2ms 后开始 */
    struct itimerval it;
    memset(&it, 0, sizeof it);
    it.it_interval.tv_usec = 1000;
    it.it_value.tv_usec    = 2000;
    if (setitimer(ITIMER_PROF, &it, NULL) != 0) { perror("setitimer"); return 1; }

    const long ROUNDS = 2000000L;
    printf("开始工作量：%ld 轮（每轮一次慢开方 + 一次格式化）\n", ROUNDS);
    printf("采样器：setitimer(ITIMER_PROF) 每 1ms CPU 时间 → SIGPROF → backtrace()\n\n");
    fflush(stdout);

    do_work(ROUNDS);

    struct itimerval zero;
    memset(&zero, 0, sizeof zero);
    setitimer(ITIMER_PROF, &zero, NULL);      /* 停表 */

    int n = (int)g_n;
    printf("\n共采到 %d 个样本（≈ %d ms 进程 CPU 时间）%s\n",
           n, n, g_dropped ? "  ⚠️ 有样本因缓冲区满被丢弃" : "");
    printf("（已跳过每个栈开头的 %d 帧：frame 0 = backtrace() 内部帧，"
           "frame 1 = 采样函数 on_prof 自己）\n\n", SKIP_FRAMES);
    if (n <= 0) return 0;

    /* 聚合：跳过采样器自己的帧（见 SKIP_FRAMES 说明） */
    char nm[160];
    for (int s = 0; s < n; s++) {
        char **syms = backtrace_symbols(g_pc[s], g_depth[s]);
        if (!syms) continue;

        char key[640]; key[0] = 0;
        for (int f = SKIP_FRAMES; f < g_depth[s]; f++) {
            short_name(syms[f], nm, sizeof nm);
            struct symcnt *e = slot_of(nm);
            e->incl++;
            if (f == SKIP_FRAMES) e->self++;     /* 被打断的那个函数 */
            if (key[0]) strncat(key, ";", sizeof key - strlen(key) - 1);
            strncat(key, nm, sizeof key - strlen(key) - 1);
        }
        if (key[0]) stack_add(key);
        free(syms);
    }

    const struct symcnt *arr[MAX_SYM];
    for (int i = 0; i < g_ntab; i++) arr[i] = &g_tab[i];

    qsort(arr, (size_t)g_ntab, sizeof arr[0], cmp_by_self);
    print_sym_table("【自用 self】≡ perf report 的 Overhead 列 —— CPU 时间花在谁身上",
                    "（只统计「被打断时正停在这个函数」的样本）",
                    arr, g_ntab, n, 0);

    qsort(arr, (size_t)g_ntab, sizeof arr[0], cmp_by_incl);
    print_sym_table("【含子调用 inclusive】≡ 火焰图里该帧的宽度 —— 调用链经过谁",
                    "（样本的栈里出现过该函数就算，所以相加会超过 100%）",
                    arr, g_ntab, n, 1);

    qsort(g_stk, (size_t)g_nstk, sizeof g_stk[0], cmp_stack);
    printf("【折叠栈 collapsed】≡ stackcollapse-perf.pl 的输出 —— 可直接喂给 flamegraph.pl\n");
    printf("（本机没有 perl/flamegraph.pl，所以这里到 folded 为止；把这段贴到\n");
    printf("  你自己的 Linux 上执行 `flamegraph.pl > flame.svg`，就是一张真火焰图）\n");
    int top = g_nstk < 12 ? g_nstk : 12;
    for (int i = 0; i < top; i++)
        printf("  %-72s %d\n", g_stk[i].key, g_stk[i].n);
    printf("  （共 %d 种不同的调用栈）\n\n", g_nstk);

    printf("读法：self 表第一名就是热点，优化它收益最大（6.1）；\n");
    printf("      inclusive 表 / folded 栈告诉你「谁调用了它」——从 main 顺着往下就是调用链，\n");
    printf("      火焰图只是把这张 folded 表画成了宽度（6.2）。\n");
    printf("      perf 干的是同一件事，只是采样源换成 PMU 硬件中断，所以还能顺带\n");
    printf("      给出 cycles / instructions / cache-misses 这些计数器（IPC 就从这儿来）。\n");
    return 0;
}
