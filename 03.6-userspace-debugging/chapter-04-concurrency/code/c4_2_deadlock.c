/*
 * c4_2_deadlock.c —— 确定性 AB-BA 死锁 + 进程内看门狗自证
 *
 * 为什么要「确定性」：
 *   教科书上的 AB-BA 死锁例子，经常跑十次有三次顺利跑完 —— 因为谁先拿到
 *   第一把锁、谁先跑到第二把锁，全看调度运气。这里用两个原子变量做一次
 *   握手：两个线程各自先拿到自己的第一把锁、通报「我拿到了」，并等对方也
 *   通报完，然后才一起去抢第二把锁。这样「A 持 lock_a 等 lock_b，B 持
 *   lock_b 等 lock_a」这个环**必然**形成，和调度无关。
 *
 * 看门狗（watchdog 线程）干三件事，等价于 gdb 的 `thread apply all bt`：
 *   1. 每 200ms 采样两个工作线程的「心跳」和「当前阶段」
 *   2. 心跳连续 3 次不变 → 判定「没有任何线程在推进」
 *   3. 用 pthread_mutex_timedlock 分别去试两把锁：
 *        两把都 ETIMEDOUT → 两把锁都被别人持着，却没有线程在推进
 *        → 环形成立，死锁实锤
 *      然后打印现场并 _exit(3)
 *
 * 用法：
 *   ./c4_2_deadlock abba      # AB-BA 反序加锁 → 死锁，看门狗报出 → 退出码 3
 *   ./c4_2_deadlock ordered   # 统一锁序     → 正常跑完 → 退出码 0
 *
 * 编译：
 *   cc -g -O1 -pthread -Wall -Wextra -o c4_2_deadlock c4_2_deadlock.c
 *
 * 注意：这个 demo 不要带 TSan 跑。TSan 抓的是「数据竞争」，而这里是
 * 死锁（活锁/环形等待），本 demo 的共享状态全部用原子变量保护过了，
 * TSan 会安静通过 —— 它**看不见**死锁。这正是笔记 4.3 说的「工具各有
 * 分工」：查竞争用 TSan，查死锁靠栈（gdb / 本 demo 的看门狗）。
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* ---------------- 两把锁：竞争的目标 ---------------- */
static pthread_mutex_t lock_a = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t lock_b = PTHREAD_MUTEX_INITIALIZER;

/* ---------------- 观测面：全部用原子，避免制造真竞争 ---------------- */
enum {
    ST_START = 0,   /* 还没开始 */
    ST_GRAB1,       /* 正在抢第一把锁 */
    ST_WAIT_PEER,   /* 已持有第一把，等对方也拿到它的第一把 */
    ST_GRAB2,       /* 已持有第一把，正在抢第二把  ← 死锁就卡在这 */
    ST_BOTH,        /* 两把都拿到，在干活 */
    ST_DONE,        /* 跑完了 */
    ST_COUNT
};

static const char *ST_NAME[ST_COUNT] = {
    "启动", "抢第一把锁", "等对方握手", "抢第二把锁(←死锁卡点)", "持有两把锁", "已完成"
};

static atomic_int g_taken[2];    /* 握手用：对方是否已拿到自己的第一把锁 */
static atomic_int g_hb[2];       /* 心跳：只有真正拿到两把锁才 +1 */
static atomic_int g_stage[2];    /* 当前阶段 */

static int g_handshake = 0;      /* 1 = 反序 + 握手（必然成环） */
static int g_rounds    = 400;    /* 每线程锁几次 */

struct warg { int first, second, id; };

static void msleep(long ms) {
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

static void *worker(void *p) {
    struct warg *w = p;
    pthread_mutex_t *L[2] = { &lock_a, &lock_b };

    for (int i = 0; i < g_rounds; i++) {
        atomic_store(&g_stage[w->id], ST_GRAB1);
        pthread_mutex_lock(L[w->first]);              /* ① 拿第一把 */

        if (g_handshake && i == 0) {
            /* 只在第一轮握手，保证环一定能形成 */
            atomic_store(&g_taken[w->id], 1);
            atomic_store(&g_stage[w->id], ST_WAIT_PEER);
            while (!atomic_load(&g_taken[1 - w->id])) msleep(2);
        }

        atomic_store(&g_stage[w->id], ST_GRAB2);
        pthread_mutex_lock(L[w->second]);             /* ② 拿第二把 ← 成环点 */

        atomic_store(&g_stage[w->id], ST_BOTH);
        atomic_fetch_add(&g_hb[w->id], 1);            /* 真正前进了一格 */
        msleep(1);                                    /* 模拟 1ms 的活 */

        pthread_mutex_unlock(L[w->second]);
        pthread_mutex_unlock(L[w->first]);
    }

    atomic_store(&g_stage[w->id], ST_DONE);
    return NULL;
}

/* ------------------------------------------------------------------ *
 * 看门狗：判定 + 取证 + 打印现场
 * ------------------------------------------------------------------ */
static const char *hold_state(int rc) {
    if (rc == ETIMEDOUT) return "被占用（timedlock 超时）";
    if (rc == 0)         return "空闲  <-- 异常！说明没人持有，那不是死锁";
    return "未知错误";
}

static void report_deadlock(int round, int h0, int h1) {
    struct timespec ts = { 0, 300 * 1000 * 1000L };   /* 300ms */
    int ra = pthread_mutex_timedlock(&lock_a, &ts);
    if (ra == 0) pthread_mutex_unlock(&lock_a);
    int rb = pthread_mutex_timedlock(&lock_b, &ts);
    if (rb == 0) pthread_mutex_unlock(&lock_b);

    printf("\n");
    printf("=========================== 看门狗判定：死锁 ===========================\n");
    printf("第 %d 轮采样起，连续 3 次采样两个线程心跳都不变 → 没有任何线程在推进\n\n", round - 2);

    printf("[1] 两个线程各自停在哪（心跳 = 只有真正拿到两把锁才 +1）\n");
    printf("    thread-A  hb=%-6d 阶段=%s\n", h0, ST_NAME[atomic_load(&g_stage[0])]);
    printf("    thread-B  hb=%-6d 阶段=%s\n", h1, ST_NAME[atomic_load(&g_stage[1])]);
    printf("    ↑ 两个心跳都停在同一个数字上不动了，阶段都停在「抢第二把锁」\n\n");

    printf("[2] 独立取证：两把锁到底在谁手里（看门狗亲自去试）\n");
    printf("    pthread_mutex_timedlock(&lock_a) → %s\n", hold_state(ra));
    printf("    pthread_mutex_timedlock(&lock_b) → %s\n", hold_state(rb));
    printf("    ↑ 两把锁同时被占，且持有者毫无进展 → 环形等待成立\n\n");

    printf("[3] 还原锁序\n");
    printf("    thread-A: 拿到 lock_a → 在等 lock_b\n");
    printf("    thread-B: 拿到 lock_b → 在等 lock_a\n");
    printf("    → A: a→b，B: b→a，两个方向相反，环闭合。这就是 AB-BA 死锁。\n\n");

    printf("[4] 同样的取证，gdb 给的是「带调用栈的版本」\n");
    printf("    (gdb) thread apply all bt\n");
    printf("    Thread 1:  #0 __lll_lock_wait_private   #1 pthread_mutex_lock\n");
    printf("               #2 worker (... 'lock_b') at c4_2_deadlock.c:<② 那一行>\n");
    printf("    Thread 2:  #0 __lll_lock_wait_private   #1 pthread_mutex_lock\n");
    printf("               #2 worker (... 'lock_a') at c4_2_deadlock.c:<② 那一行>\n");
    printf("    本 demo 只打印「阶段字符串」，是因为 backtrace() 拿不到**别的线程**的栈；\n");
    printf("    gdb 能读到 .symtab，所以能直接给出函数名和行号 —— 这就是为什么卡死时\n");
    printf("    第一动作是 `thread apply all bt`（见 4.1）。\n\n");

    printf("[5] 修法：统一加锁顺序（都先 lock_a 再 lock_b），或用\n");
    printf("    pthread_mutex_trylock + 超时回退打破环。\n");
    printf("    验证：./c4_2_deadlock ordered —— 同一个程序，锁序一统一就不死了。\n");
    printf("========================================================================\n");
    fflush(stdout);
    _exit(3);   /* 看门狗判定死锁 → 退出码 3（不是崩溃，是「有人主动判定它死了」） */
}

static void *watchdog(void *arg) {
    (void)arg;
    int prev0 = -1, prev1 = -1, stalled = 0, round = 0;

    for (round = 0; round < 25; round++) {
        msleep(200);
        int h0 = atomic_load(&g_hb[0]);
        int h1 = atomic_load(&g_hb[1]);
        int s0 = atomic_load(&g_stage[0]);
        int s1 = atomic_load(&g_stage[1]);

        printf("[watchdog 第%2d轮] A: hb=%-5d %-22s | B: hb=%-5d %-22s\n",
               round, h0, ST_NAME[s0], h1, ST_NAME[s1]);
        fflush(stdout);

        if (s0 == ST_DONE && s1 == ST_DONE) {
            printf("\n[watchdog] 两个线程都已 ST_DONE，心跳从 0 涨到 %d —— 全程有推进，无死锁。\n", h0);
            fflush(stdout);
            return NULL;
        }

        if (h0 == prev0 && h1 == prev1) {
            if (++stalled >= 3) report_deadlock(round, h0, h1);
        } else {
            stalled = 0;
        }
        prev0 = h0;
        prev1 = h1;
    }
    printf("\n[watchdog] 观察窗口结束，未判定死锁。\n");
    return NULL;
}

int main(int argc, char **argv) {
    const char *mode = (argc > 1) ? argv[1] : "abba";

    if (strcmp(mode, "abba") == 0) {
        g_handshake = 1;
        printf("模式 = abba（反序加锁 + 握手确保成环）\n");
    } else if (strcmp(mode, "ordered") == 0) {
        g_handshake = 0;
        printf("模式 = ordered（两个线程都按 lock_a → lock_b 的顺序拿锁）\n");
    } else {
        fprintf(stderr, "用法: %s {abba|ordered}\n", argv[0]);
        return 2;
    }
    printf("每线程锁 %d 轮，每轮临界区里干 1ms 的活；看门狗每 200ms 采样一次。\n", g_rounds);
    fflush(stdout);

    /* ordered 模式下两个线程锁序相同；abba 模式下第二个线程反过来 */
    static struct warg wa = { 0, 1, 0 };   /* a → b */
    static struct warg wb_abba = { 1, 0, 1 };  /* b → a  ← 反序 */
    static struct warg wb_ordered = { 0, 1, 1 };

    pthread_t t0, t1, tw;
    pthread_create(&tw, NULL, watchdog, NULL);
    pthread_create(&t0, NULL, worker, &wa);
    pthread_create(&t1, NULL, worker, g_handshake ? &wb_abba : &wb_ordered);

    if (!g_handshake) {
        /* ordered 模式会正常跑完 */
        pthread_join(t0, NULL);
        pthread_join(t1, NULL);
        pthread_join(tw, NULL);
        printf("\n主线程：两个 worker 都正常返回，退出码 0。\n");
        return 0;
    }

    /* abba 模式永远不会走到这里（看门狗会 _exit(3)） */
    pthread_join(tw, NULL);
    return 0;
}
