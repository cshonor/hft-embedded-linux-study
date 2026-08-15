# P5d — 多线程传感器融合 + 延迟 p99 统计

> 多个传感器并发采样、线程融合数据，统计端到端延迟分布（p50/p99/p999），把嵌入式实时性量化。
> **做法：项目驱动，[`10`](../../../10-embedded-projects/) / [`16`](../../../15-systems-performance/) 笔记当字典。**

---

## 最小预备

| 瞄一眼 | 只要留下印象 |
|--------|-------------|
| [Pi5 Labs](../../../10-embedded-projects/RASPBERRY-PI5-LABS.md) | 板级动手清单 |
| [CSAPP 12.5 信号量与预线程化](../../../02-computer-systems/chapter-12-concurrent-programming/notes/section-12.5-信号量与预线程化.md) | 线程池同步基础 |
| [C++ Concurrency ch03](../../../04-cpp/M2-deep-principles/02-Cpp-Concurrency/ch03-sharing-data/) | mutex/atomic |

---

## 项目目标

把"多线程 + 延迟统计"做到嵌入式场景：传感器线程采集、融合线程合并、输出线程下发，测量从采样到输出的延迟分布。这也是 HFT 延迟统计方法在嵌入式侧的预演。

## Phase 1：单线程基线（1 小时）

### 做什么

单线程轮询两个传感器，直接输出，建立基线延迟。

### 代码骨架

```c
#include <time.h>
#include <stdio.h>

// 传感器数据 + 时间戳
struct sensor_sample {
    int accel[3];   // 加速度 XYZ
    int gyro[3];    // 陀螺仪 XYZ
    uint64_t ts;    // 采样时刻 (CLOCK_MONOTONIC ns)
};

// 单线程：读 → 融合 → 输出
void single_thread_loop(void) {
    struct sensor_sample s;
    for (int i = 0; i < 10000; i++) {
        uint64_t t_start = get_time_ns();
        read_accel(s.accel);     // 从 P5c 驱动读
        read_gyro(s.gyro);
        s.ts = get_time_ns();
        // 简单融合：直接合并
        struct output out = fuse(&s);
        uint64_t t_end = get_time_ns();
        uint64_t latency = t_end - s.ts;
        record_latency(latency);
    }
}

uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}
```

### 分步实现

1. **时间戳函数**：`clock_gettime(CLOCK_MONOTONIC, &ts)` → 纳秒
2. **读传感器**：复用 P5c 的用户态读取接口
3. **打点**：采样时刻、融合完成、输出时刻
4. **延迟统计**：记录 10000 次延迟，排序算 p50/p99/p999

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| 用 `CLOCK_REALTIME` | 时间跳变 | 用 `CLOCK_MONOTONIC`（不受 NTP 影响）|
| `clock_gettime` 频率太低 | 精度差 | 用 `CLOCK_MONOTONIC_RAW` 或 TSC（P2.5 交付物 8）|

---

## Phase 2：多线程 + 锁（1-2 小时）

### 做什么

三线程架构：采集线程 → 共享缓冲 + mutex → 融合线程 → 共享缓冲 + mutex → 输出线程。

### 代码骨架

```c
#include <pthread.h>

// 共享缓冲
struct shared_buf {
    struct sensor_sample data[256];
    int head, tail;
    pthread_mutex_t lock;
    pthread_cond_t not_empty, not_full;
};

// 采集线程
void *collector(void *arg) {
    struct shared_buf *buf = arg;
    for (;;) {
        struct sensor_sample s;
        read_sensors(&s);
        pthread_mutex_lock(&buf->lock);
        while (buf_full(buf))  // while 不是 if
            pthread_cond_wait(&buf->not_full, &buf->lock);
        buf_push(buf, &s);
        pthread_cond_signal(&buf->not_empty);
        pthread_mutex_unlock(&buf->lock);
    }
}

// 融合线程
void *fuse_worker(void *arg) {
    struct shared_buf *in = arg_in;
    struct shared_buf *out = arg_out;
    for (;;) {
        struct sensor_sample s;
        pthread_mutex_lock(&in->lock);
        while (buf_empty(in))
            pthread_cond_wait(&in->not_empty, &in->lock);
        buf_pop(in, &s);
        pthread_cond_signal(&in->not_full);
        pthread_mutex_unlock(&in->lock);

        struct output result = fuse(&s);  // 融合

        // 推到输出缓冲
        pthread_mutex_lock(&out->lock);
        while (buf_full(out))
            pthread_cond_wait(&out->not_full, &out->lock);
        buf_push(out, &result);
        pthread_cond_signal(&out->not_empty);
        pthread_mutex_unlock(&out->lock);
    }
}
```

### 分步实现

1. **采集线程**：读传感器 → 加锁 → 入队 → 通知 → 解锁
2. **融合线程**：加锁 → 等数据 → 出队 → 通知 → 解锁 → 融合 → 入输出队列
3. **输出线程**：出队 → 记录延迟 → 下发
4. **统计**：对比 Phase 1 基线，看锁竞争对延迟的影响

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| 锁竞争严重 | p99 暴涨 | 两个线程同时抢一把锁 |
| 条件变量丢唤醒 | 线程永久阻塞 | `while` 不是 `if`（spurious wakeup）|
| 优先级反转 | 高优先级线程等低优先级 | 用 `PTHREAD_PRIO_INHERIT` 或无锁队列 |

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| 线程同步 | [TLPI ch30](../../../03-linux-userspace-api/chapter-30-thread-synchronization/) |
| 预线程化 | [CSAPP 12.5](../../../02-computer-systems/chapter-12-concurrent-programming/notes/section-12.5-信号量与预线程化.md) |

---

## Phase 3：换无锁队列对比 p99（1 小时）

### 做什么

把 Phase 2 的 mutex 队列换成 P2.5 的 SPSC 无锁 ring buffer，对比 p99 延迟。

### 分步实现

1. **采集→融合**：SPSC ring buffer（单写单读，无需锁）
2. **融合→输出**：另一个 SPSC ring buffer
3. **对比**：
   - Phase 1（单线程）：p50/p99 基线
   - Phase 2（多线程+锁）：p99 应该比基线高（锁开销+调度）
   - Phase 3（多线程+无锁）：p99 应该比 Phase 2 低
4. **看尾延迟**：重点关注 p99/p999，不是平均值——HFT 看的是最坏情况

### 为什么重要

这就是 HFT 和内核的核心取舍：mutex 简单但有调度开销和优先级反转风险；无锁队列复杂但尾延迟可控。你在这里量化的就是两者的差异。

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| 无锁队列原理 | [C++ Concurrency ch07](../../../04-cpp/M2-deep-principles/02-Cpp-Concurrency/ch07-lock-free-containers/) |
| 你写的 ring buffer | P2.5 交付物 3（直接复用代码！）|

---

## Phase 4：PREEMPT_RT 对比尾延迟（1 小时）

### 做什么

换 PREEMPT_RT 内核，对比普通内核的尾延迟改善。

### 分步实现

1. **编译 PREEMPT_RT 内核**：`make menuconfig` → `Preemption Model` → `Fully Preemptible Kernel (Real-Time)`
2. **运行 Phase 3 的程序**，记录延迟分布
3. **对比**：
   - 普通内核：p99 可能 100us+，p999 可能 ms 级
   - PREEMPT_RT：p99 应该 < 50us，尾延迟更稳定
4. **用 `chrt` 设优先级**：`sudo chrt -f 99 ./sensor_fusion`

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| PREEMPT_RT 内核没装 | 行为跟普通内核一样 | 确认 `uname -a` 有 `PREEMPT RT` |
| 没设实时优先级 | 延迟仍抖动 | `chrt -f 99` 设 SCHED_FIFO |
| 中断干扰 | 偶发高延迟 | `isolcpus` 隔离 CPU，`irqbalance` 绑定 |

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| 调度延迟 | [ULK ch07 调度](../../../19-linux-kernel-deep/chapter-07-process-scheduling/) |
| 性能分析 | [16 SysPerf](../../../15-systems-performance/) |

---

## 延迟统计代码

```c
// 直方图统计
#define NUM_BUCKETS 64
static uint64_t latency_buckets[NUM_BUCKETS];
static uint64_t latency_max = 0;

void record_latency(uint64_t ns) {
    if (ns > latency_max) latency_max = ns;
    // 对数桶：0-100ns, 100-200ns, ..., 10ms+
    int bucket = 0;
    uint64_t threshold = 100;
    while (ns > threshold && bucket < NUM_BUCKETS - 1) {
        threshold *= 2;
        bucket++;
    }
    latency_buckets[bucket]++;
}

void print_stats(void) {
    uint64_t total = 0;
    for (int i = 0; i < NUM_BUCKETS; i++) total += latency_buckets[i];

    // 算 p50/p99/p999
    uint64_t cumulative = 0;
    for (int i = 0; i < NUM_BUCKETS; i++) {
        cumulative += latency_buckets[i];
        double pct = (double)cumulative / total * 100;
        if (pct >= 50 && !p50_done) printf("p50: <%lu ns\n", (1UL << i) * 100);
        if (pct >= 99 && !p99_done) printf("p99: <%lu ns\n", (1UL << i) * 100);
        if (pct >= 99.9 && !p999_done) printf("p999: <%lu ns\n", (1UL << i) * 100);
    }
    printf("max: %lu ns\n", latency_max);
}
```

## 状态

⬜ 未开始 → 建议先跑 Phase 1 基线，建立"延迟是多少"的直觉。

← [P5 索引](../README.md) · [13 模块](../../../10-embedded-projects/) · [19 模块](../../../15-systems-performance/)
