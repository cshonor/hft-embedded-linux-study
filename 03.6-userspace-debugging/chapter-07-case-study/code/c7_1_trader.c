/*
 * c7_1_trader.c —— Ch7 贯穿示例：迷你下单引擎，一个程序埋四类雷
 *
 * 架构（与笔记 7.1 的图一致）：
 *
 *        ┌──────────────────────────────┐
 *        │      共享订单簿 g_book         │
 *        │  （链表，g_book_lock 保护）    │
 *        └──────┬────────────────┬──────┘
 *     加锁插入  │                │  加锁摘除
 *      ┌────────▼──────┐  ┌──────▼──────────┐
 *      │ feed 线程      │  │ match 线程       │
 *      │ malloc 订单 →  │  │ 摘头部订单 →     │
 *      │ 塞进订单簿头    │  │ 累计 g_total →   │
 *      └───────────────┘  │ free 订单        │
 *                          └─────────────────┘
 *
 * 默认（不加宏）= 正确版本，结果是 g_total = 40100。
 * 四类雷用宏独立开启，**同一份骨架，只换破坏点**（控制变量法，见 1.3）：
 *   -DBUG_CRASH  崩溃：feed 里拿订单 id 当数组下标，id 最大 200 远超 16 → 越界写栈
 *   -DBUG_RACE   竞态：feed 与 match 无锁累加 g_total
 *   -DBUG_LEAK   泄漏：match 摘除订单后不 free
 *   -DBUG_HANG   卡住：feed 同一线程重复锁 g_book_lock（非递归锁 → 自死锁）
 *   -DFIX_RUNNING 修复：把 g_running 从 volatile 换成 C11 原子（见下面的长注释）
 *
 * 编译 / 运行：
 *   cc -g -O0 -pthread -Wall -Wextra -o c7_1_base   c7_1_trader.c
 *   cc -g -O0 -pthread -Wall -Wextra -fstack-protector-all -DBUG_CRASH -o c7_1_crash c7_1_trader.c
 *   cc -g -O1 -pthread -fsanitize=thread  -DBUG_RACE -o c7_1_race c7_1_trader.c
 *   cc -g -O1 -pthread -fsanitize=address -DBUG_LEAK -o c7_1_leak c7_1_trader.c
 *   cc -g -O0 -pthread -Wall -Wextra -DBUG_HANG -o c7_1_hang c7_1_trader.c
 *   cc -g -O1 -pthread -fsanitize=thread -DFIX_RUNNING -o c7_1_fixed c7_1_trader.c
 *   #                          ↑ 基线 + TSan 会报 1 条；加 -DFIX_RUNNING 后静默
 *
 * 为了「在容器里也能演示卡住」，本程序比笔记里的版本多了一个**看门狗**：
 *   main 里 alarm(3)，超过 3 秒没结束就由 SIGALRM 处理函数打印现场并 _exit(4)。
 *   理由：CE 沙箱里没有 `timeout` 命令，卡死的进程没法收场；
 *   而且「进程自己报警」这件事本身也有教学价值 —— 生产环境里正是这么干的。
 *   正常的基线跑 200 轮 × 1ms，不到 1 秒就结束，看门狗根本不会触发。
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define N_ORDERS       200
#define WATCHDOG_SEC     3

/* ---------------- g_running 的两种写法 ----------------
 * 默认沿用笔记里的 `volatile int g_running`。但 ⚠️ 实测发现：这样写**本身就是数据竞争**——
 * feed 无锁写 `g_running = 0`，match 在 `while (g_running || g_book)` 里无锁读它。
 * `volatile` 只保证「每次都真的访存」，**不提供任何原子性或 happens-before**（和 3.2 一样）。
 * 后果：TSan 在「正确版本」上照样报 1 条 data race，退出码 66。
 *
 *   实测（clang 18.1.0 + TSan，基线代码）：
 *     WARNING: ThreadSanitizer: data race
 *       Write of size 4 by thread T1:  #0 feed_thread  example.c:167   ← g_running = 0
 *       Previous read of size 4 by T2: #0 match_thread example.c:175   ← while (g_running ...)
 *       Location is global 'g_running'
 *     ThreadSanitizer: reported 1 warnings     退出码 66
 *
 * 加上 -DFIX_RUNNING 就是修好的版本：g_running 换成 C11 原子，且退出条件挪进锁内
 * （这样连 `g_book` 的无锁读也一起消掉）。实测 TSan 静默、退出码 0。
 * 这正是本章要教的事：**「结果对」和「没有数据竞争」是两件事**。 */
#ifdef FIX_RUNNING
static atomic_int g_running = 1;               /* 修复版：原子，不是 volatile */
#  define RUNNING_SET(v)      atomic_store(&g_running, (v))
#  define RUNNING_GET()       atomic_load(&g_running)
#else
volatile int g_running = 1;                    /* 默认版：笔记同款，本身就有竞争 */
#  define RUNNING_SET(v)      do { g_running = (v); } while (0)
#  define RUNNING_GET()       (g_running)
#endif

/* 用 nanosleep 而不是 usleep：_POSIX_C_SOURCE=200809L 下 usleep 已被 POSIX 移除、
 * glibc 不再声明它（gcc 给个 implicit-declaration 警告还能过，clang 直接报错）。 */
static void sleep_us(long us)
{
    struct timespec ts;
    ts.tv_sec  = us / 1000000;
    ts.tv_nsec = (us % 1000000) * 1000L;
    nanosleep(&ts, NULL);
}

typedef struct order {
    int    id;
    double price;
    long   qty;
    struct order *next;
} order_t;

order_t *g_book = NULL;
pthread_mutex_t g_book_lock = PTHREAD_MUTEX_INITIALIZER;  /* 保护订单簿链表 */
pthread_mutex_t g_stat_lock = PTHREAD_MUTEX_INITIALIZER;  /* 保护统计量 */

/* g_running 在文件上半部分按 FIX_RUNNING 二选一声明 */
volatile long g_total   = 0;      /* 累计撮合 qty，正确基线 = 40100 */

/* 观测面：进度与分配计数。feed 只写 feed_*，match 只写 match_*，彼此不冲突 */
static volatile long g_feed_progress  = 0;
static volatile long g_match_progress = 0;
static volatile int  g_feed_done      = 0;
static volatile int  g_match_done     = 0;
static long g_alloc = 0, g_freed = 0;

/* ------------------------------------------------------------------ *
 * 看门狗：证明「卡住」而不只是「慢」
 * ------------------------------------------------------------------ */
static void on_alarm(int sig)
{
    (void)sig;
    /* ⚠️ 严格说 snprintf 不是 async-signal-safe；这里它是安全的（不分配内存），
     *    真正的生产代码在信号处理函数里只该用 write()。见 2.3。 */
    char buf[512];
    int n = snprintf(buf, sizeof buf,
        "\n[看门狗] alarm 已经 %d 秒 —— 进程没结束，判定卡住。\n"
        "  feed  线程：进度 %ld/%d（%s）\n"
        "  match 线程：进度 %ld（%s）\n",
        WATCHDOG_SEC,
        (long)g_feed_progress, N_ORDERS, g_feed_done ? "已返回" : "★仍在跑★",
        (long)g_match_progress, g_match_done ? "已返回" : "★仍在跑★");
    if (n > 0) (void)!write(STDERR_FILENO, buf, (size_t)n);

    /* 用 trylock 探测锁的归属：能拿到说明锁空闲，EBUSY 说明锁被占着 */
    int rc = pthread_mutex_trylock(&g_book_lock);
    if (rc == EBUSY) {
        const char *m =
            "  g_book_lock: EBUSY —— 锁被持有，但两个线程都不再推进\n"
            "               → 持有者自己在等自己，自死锁实锤\n"
            "  （7.5 里 strace 会给同一件事的另一种视角：所有线程都停在 futex 上等锁）\n";
        (void)!write(STDERR_FILENO, m, strlen(m));
    } else {
        const char *m = "  g_book_lock: 空闲 —— 那就不是锁死问题，得看别处\n";
        (void)!write(STDERR_FILENO, m, strlen(m));
        pthread_mutex_unlock(&g_book_lock);
    }
    {
        /* ⚠️ write() 的第三个参数必须用 strlen：手写数字第一版写成 12，
         *    结果字符串被截断成 "  → _exit("。write 不看 NUL，只认长度。 */
        const char *m = "  → 看门狗 _exit(4)（不是崩溃，是有人主动判定它死了）\n";
        (void)!write(STDERR_FILENO, m, strlen(m));
    }
    _exit(4);
}

/* ------------------------------------------------------------------ */

static void *feed_thread(void *arg)
{
    (void)arg;
    for (int i = 1; i <= N_ORDERS; i++) {
        order_t *o = malloc(sizeof(order_t));
        if (!o) break;
        o->id    = i;
        o->price = (double)(i * 10);
        o->qty   = 100 + i;
        g_alloc++;

#ifdef BUG_CRASH
        /* 雷1（崩溃）：拿 id 当数组下标。id 一路涨到 200，数组只有 16 个元素
         *             → 最远写出 800 字节，直接踩烂 feed_thread 的栈帧 */
        {
            int slots[16];
            slots[o->id] = 1;
        }
#endif

#ifdef BUG_HANG
        pthread_mutex_lock(&g_book_lock);   /* 第一次：拿到 */
        pthread_mutex_lock(&g_book_lock);   /* 雷4：同一线程再锁非递归锁 → 自死锁 */
        o->next = g_book;                   /* 永远到不了这里 */
        g_book = o;
        pthread_mutex_unlock(&g_book_lock);
        pthread_mutex_unlock(&g_book_lock);
#else
        pthread_mutex_lock(&g_book_lock);
        o->next = g_book;
        g_book  = o;
        pthread_mutex_unlock(&g_book_lock);
#endif

#ifdef BUG_RACE
        g_total += o->qty;                  /* 雷2：feed 无锁写 g_total */
#endif

        g_feed_progress = i;
        sleep_us(1000);
    }
    RUNNING_SET(0);            /* 默认版：无锁写 volatile（TSan 会报）；修复版：atomic_store */
    g_feed_done = 1;
    return NULL;
}

static void *match_thread(void *arg)
{
    (void)arg;
    /* 退出条件放进锁内判定：这样 `g_book` 的读受 g_book_lock 保护，
     * 不会出现「在 while 条件里无锁读 g_book」这个隐藏竞争。
     * （默认版还有一个竞争消不掉：feed 无锁写 g_running —— 那是 -DFIX_RUNNING 修的东西。） */
    for (;;) {
        pthread_mutex_lock(&g_book_lock);
        order_t *o = g_book;
        int stop = (!o && !RUNNING_GET());       /* 没有存量订单 && feed 已收工 */
        if (o) {
            g_book = o->next;

#ifdef BUG_RACE
            g_total += o->qty;              /* 雷2：match 无锁写 g_total（与 feed 竞争） */
#else
            pthread_mutex_lock(&g_stat_lock);
            g_total += o->qty;
            pthread_mutex_unlock(&g_stat_lock);
#endif

#ifdef BUG_LEAK
            /* 雷3（泄漏）：成交订单摘除后不 free。把下面这行的注释去掉就修好了。 */
            /* free(o); g_freed++; */
#else
            free(o);
            g_freed++;
#endif
        }
        pthread_mutex_unlock(&g_book_lock);

        if (stop) break;
        g_match_progress++;
        sleep_us(500);
    }
    g_match_done = 1;
    return NULL;
}

int main(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_alarm;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, NULL);
    alarm(WATCHDOG_SEC);

    printf("迷你下单引擎：feed 塞 %d 单，match 撮合；正确基线 = 40100\n", N_ORDERS);
    printf("变体：");
    {
        int any = 0;
#ifdef BUG_CRASH
        printf(" CRASH"); any = 1;
#endif
#ifdef BUG_RACE
        printf(" RACE"); any = 1;
#endif
#ifdef BUG_LEAK
        printf(" LEAK"); any = 1;
#endif
#ifdef BUG_HANG
        printf(" HANG"); any = 1;
#endif
        if (!any) printf(" 无（正确版本）");
    }
    printf("\n\n");
    fflush(stdout);

    pthread_t feed, match;
    pthread_create(&feed,  NULL, feed_thread,  NULL);
    pthread_create(&match, NULL, match_thread, NULL);
    pthread_join(feed,  NULL);
    pthread_join(match, NULL);

    alarm(0);   /* 正常收工，撤掉看门狗 */

    printf("total matched qty = %ld   （期望 40100）\n", g_total);
    printf("订单计数：malloc %ld 个，free %ld 个，仍存活 %ld 个（每个 %zu 字节，约 %ld 字节）\n",
           g_alloc, g_freed, g_alloc - g_freed,
           sizeof(order_t), (g_alloc - g_freed) * (long)sizeof(order_t));
    printf("进程进度：feed 到 %ld/%d，match 走了 %ld 轮\n",
           (long)g_feed_progress, N_ORDERS, (long)g_match_progress);

    if (g_alloc != g_freed)
        printf("\n注意：进程**正常退出**，退出码 0，但 %ld 个订单永远收不回来了。\n"
               "      泄漏不会崩、不会卡、不会报错 —— 只有内存曲线一直往上爬（见 7.4）。\n",
               g_alloc - g_freed);
    return 0;
}
