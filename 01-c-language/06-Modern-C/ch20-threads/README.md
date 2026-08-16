# Ch20 · Threads（线程）

> **Level 3 · 深入** · 策略：**🔴 精读**（进 DPDK 前必读）
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

## 本章讲什么

C11 `threads.h`（`thrd_create`/`join`/`detach`）、线程局部数据（`tss`/`_Thread_local`）、
互斥锁（`mtx`）、条件变量（`cnd`）、线程管理策略。
**DPDK 每 lcore 一线程模型 = `_Thread_local` + 绑核**，虽然生产用 pthread，
但 `threads.h` 是理解模型的最短路径。

## 一、C11 `threads.h` 概览

C11 把线程纳入标准库（`<threads.h>`），不再依赖 POSIX。

| API | 功能 | POSIX 对应 |
|-----|------|-----------|
| `thrd_create(&t, fn, arg)` | 创建线程 | `pthread_create` |
| `thrd_join(t, &res)` | 等待线程结束 | `pthread_join` |
| `thrd_detach(t)` | 分离线程（自动回收） | `pthread_detach` |
| `thrd_exit(res)` | 线程退出 | `pthread_exit` |
| `thrd_sleep(&ts, NULL)` | 线程睡眠 | `nanosleep` |
| `thrd_yield()` | 让出 CPU | `sched_yield` |

### 创建线程

```c
#include <threads.h>
#include <stdio.h>

int worker(void *arg) {
    int id = *(int *)arg;
    printf("thread %d running\n", id);
    return 0;
}

int main(void) {
    thrd_t t;
    int id = 42;
    thrd_create(&t, worker, &id);   // 创建线程
    thrd_join(t, NULL);             // 等待结束
    return 0;
}
```

| 要点 | 说明 |
|------|------|
| 线程函数签名 | `int (*)(void *)` — 接收 `void*` 参数，返回 int |
| `thrd_create` 返回值 | `thrd_success` / `thrd_nomem` / `thrd_error` |
| `thrd_join` | 阻塞直到线程结束，可获取返回值 |
| `thrd_detach` | 分离后不能 join，线程结束时自动回收资源 |

### `thrd_join` vs `thrd_detach`

```c
/* 方式1: join — 主线程等待子线程结束 */
thrd_t t;
thrd_create(&t, worker, &id);
// ... 主线程做其它事
thrd_join(t, NULL);    // 阻塞直到 t 结束

/* 方式2: detach — 子线程自动回收，主线程不等 */
thrd_t t;
thrd_create(&t, worker, &id);
thrd_detach(t);        // 分离：结束后自动回收，不能 join
// 主线程继续，不等待 t
```

| 选择 | 适用场景 |
|------|----------|
| `thrd_join` | 需要线程返回值；需要确保线程完成（如初始化线程） |
| `thrd_detach` | 后台守护线程；不需要返回值；不需要等待完成 |

> **HFT 模式**：主线程创建 worker 线程 → join 等待所有 worker 完成 → 清理退出。不用 detach（需要控制生命周期）。

## 二、线程局部数据

### `_Thread_local`（编译期）

```c
/* C11: _Thread_local / C23: thread_local */
static _Thread_local unsigned lcore_id;      // 每线程独立
static _Thread_local struct {
    uint64_t rx_pkts;
    uint64_t tx_pkts;
} lcore_stats;                               // 每线程独立统计

int worker(void *arg) {
    lcore_id = *(unsigned *)arg;             // 设置当前线程的 lcore_id
    // lcore_stats 自动零初始化
    while (running) {
        lcore_stats.rx_pkts += do_rx();
    }
    return 0;
}
```

| 特性 | 说明 |
|------|------|
| 每线程独立副本 | 线程间互不干扰，无需同步 |
| 零初始化 | 像静态变量一样自动清零 |
| 编译期确定 | 不能动态改变 |
| 性能 | 访问速度接近全局变量（TLS 段寄存器） |

### TSS — 线程特定存储（运行期）

```c
/* C11: tss_t / tss_create / tss_set / tss_get */
tss_t key;

void destructor(void *val) {
    free(val);    // 线程退出时自动调用
}

int worker(void *arg) {
    /* 每线程分配自己的缓冲区 */
    char *buf = malloc(1024);
    tss_set(key, buf);           // 绑定到当前线程

    /* 其它代码可以通过 tss_get 获取 */
    char *my_buf = tss_get(key);

    return 0;    // 线程退出时 destructor 自动 free(buf)
}

int main(void) {
    tss_create(&key, destructor);   // 创建 TSS key，指定析构函数
    // ... 创建线程 ...
    tss_delete(key);                // 删除 key
}
```

| `_Thread_local` vs `tss_t` | 区别 |
|----------------------------|------|
| `_Thread_local` | 编译期声明，静态生命周期，零初始化，快 |
| `tss_t` | 运行期创建，动态绑定值，有析构回调，慢但灵活 |
| HFT 选择 | 优先 `_Thread_local`（简单、快），只有需要动态分配/析构时用 `tss_t` |

### DPDK lcore 模型

```
DPDK 的线程模型：每个 lcore = 一个线程，绑核到物理 CPU

┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐
│  CPU 0   │  │  CPU 1   │  │  CPU 2   │  │  CPU 3   │
│ (lcore 0)│  │ (lcore 1)│  │ (lcore 2)│  │ (lcore 3)│
│          │  │          │  │          │  │          │
│ RX 线程  │  │ Worker 1 │  │ Worker 2 │  │ TX 线程  │
│ _Thread_ │  │ _Thread_ │  │ _Thread_ │  │ _Thread_ │
│ local 数据│  │ local 数据│  │ local 数据│  │ local 数据│
└──────────┘  └──────────┘  └──────────┘  └──────────┘
      │                            │             ↑
      └─────── rte_ring ───────────┘─────────────┘
              (无锁队列连接各 lcore)
```

```c
/* DPDK 实际用 pthread，但模型与 threads.h 一致 */
static int
lcore_worker(void *arg)
{
    unsigned lcore_id = rte_lcore_id();
    struct lcore_conf *conf = &lcore_conf[lcore_id];

    while (!should_stop) {
        /* 从 RX ring 收包 → 处理 → 放入 TX ring */
        uint16_t n = rte_ring_dequeue_burst(conf->rx_ring, ...);
        if (n > 0) {
            process_packets(conf, n);
            rte_ring_enqueue_burst(conf->tx_ring, ...);
        }
    }
    return 0;
}

/* DPDK 的 rte_eal_remote_launch 就是 thrd_create + 绑核 */
rte_eal_remote_launch(lcore_worker, NULL, lcore_id);
```

| 概念 | C11 `threads.h` | DPDK | 内核 |
|------|-----------------|------|------|
| 线程创建 | `thrd_create` | `rte_eal_remote_launch` (封装 pthread) | `kthread_create` |
| 线程局部数据 | `_Thread_local` | `RTE_PER_LCORE` | `DEFINE_PER_CPU` |
| 绑核 | （不直接支持） | `rte_thread_set_affinity()` | `set_cpus_allowed_ptr` |

## 三、互斥锁（Mutex）

### C11 `mtx_t`

```c
#include <threads.h>

mtx_t lock;
int shared_counter = 0;

int worker(void *arg) {
    for (int i = 0; i < 1000000; i++) {
        mtx_lock(&lock);          // 加锁
        shared_counter++;          // 临界区：安全修改共享数据
        mtx_unlock(&lock);        // 解锁
    }
    return 0;
}

int main(void) {
    mtx_init(&lock, mtx_plain);   // 初始化（普通锁）

    thrd_t t1, t2;
    thrd_create(&t1, worker, NULL);
    thrd_create(&t2, worker, NULL);
    thrd_join(t1, NULL);
    thrd_join(t2, NULL);

    printf("counter = %d\n", shared_counter);  // 一定是 2000000
    mtx_destroy(&lock);
    return 0;
}
```

### Mutex 类型

| 类型 | 说明 |
|------|------|
| `mtx_plain` | 普通锁 |
| `mtx_timed` | 支持超时（`mtx_timedlock`） |
| `mtx_plain \| mtx_recursive` | 递归锁（同一线程可多次加锁） |
| `mtx_timed \| mtx_recursive` | 超时 + 递归 |

### 临界区规则

| 规则 | 说明 |
|------|------|
| 临界区尽量短 | 持锁时间越长， contention 越严重 |
| 不在锁内做 IO | `printf`/`write` 可能阻塞，导致锁持有时间不可控 |
| 不在锁内调用未知代码 | 回调函数可能再次加锁 → 死锁 |
| 固定加锁顺序 | 多把锁时按固定顺序获取，避免死锁 |

```c
/* ❌ 死锁：交叉加锁 */
void transfer(account *a, account *b) {
    mtx_lock(&a->lock);
    mtx_lock(&b->lock);    // 如果另一个线程 transfer(b, a) → 死锁
    // ...
}

/* ✅ 按地址排序加锁 */
void transfer(account *a, account *b) {
    if (a < b) { mtx_lock(&a->lock); mtx_lock(&b->lock); }
    else       { mtx_lock(&b->lock); mtx_lock(&a->lock); }
    // ...
}
```

> **HFT 立场**：热路径不用互斥锁！用无锁数据结构（`_Atomic` + 内存序，见 Ch21）或
> 每 lcore 独立数据（`_Thread_local`，避免共享）。锁只在初始化/配置阶段使用。

## 四、条件变量（Condition Variable）

### C11 `cnd_t`

```c
#include <threads.h>

mtx_t mtx;
cnd_t cv;
int data_ready = 0;

/* 生产者 */
int producer(void *arg) {
    mtx_lock(&mtx);
    data_ready = 1;
    cnd_signal(&cv);        // 通知等待的消费者
    mtx_unlock(&mtx);
    return 0;
}

/* 消费者 */
int consumer(void *arg) {
    mtx_lock(&mtx);
    while (!data_ready) {
        cnd_wait(&cv, &mtx);   // 释放锁 + 等待信号 + 重新加锁
    }
    // data_ready == 1，安全处理
    mtx_unlock(&mtx);
    return 0;
}
```

### `cnd_wait` 的工作原理

```
cnd_wait(&cv, &mtx) 做了三件事：
1. 原子地释放 mtx 并进入等待状态（不会"释放锁"和"等待"之间有窗口）
2. 被唤醒后重新获取 mtx
3. 返回时持锁

为什么用 while 而不是 if？
→ 虚假唤醒 (spurious wakeup)：cnd_wait 可能在没有 signal 的情况下返回
→ 必须重新检查条件
```

| 要点 | 说明 |
|------|------|
| `cnd_wait` 必须在 `while` 循环中 | 防止虚假唤醒 |
| `cnd_signal` 唤醒一个等待者 | 如果有多个消费者 |
| `cnd_broadcast` 唤醒所有等待者 | 广播通知 |
| 必须配合 mutex 使用 | 条件变量本身不保证条件的安全性 |

> **HFT 注意**：条件变量有上下文切换开销（~微秒级），热路径不用。DPDK 用轮询（polling）模式——
> 线程始终在 `while` 循环中检查 ring buffer，不睡眠不等待，以延迟换吞吐。

## 五、线程管理策略

### 生产环境：pthread vs threads.h

| 方面 | C11 `threads.h` | POSIX `pthread` |
|------|-----------------|-----------------|
| 可移植性 | C11 标准（理论可移植） | POSIX（Linux/Unix 事实标准） |
| CPU 亲和性 | ❌ 不支持 | ✅ `pthread_setaffinity_np` |
| 实时调度 | ❌ 不支持 | ✅ `pthread_setschedparam` |
| 信号掩码 | ❌ 不支持 | ✅ `pthread_sigmask` |
| 实现质量 | 参差（glibc 的 thrd 封装 pthread） | 成熟稳定 |
| HFT 选择 | 学习模型用 | **生产用**（绑核、调度、信号都需要） |

```c
/* DPDK 实际用的 pthread 绑核代码 */
#include <pthread.h>
#include <sched.h>

void pin_to_core(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
}
```

### DPDK 轮询线程模型

```
传统线程模型（阻塞/睡眠）：
  while (running) {
      data = recv();        // 阻塞等待数据
      process(data);
  }
  ↑ 线程大部分时间在睡眠，有唤醒延迟（微秒级）

DPDK 轮询模型（100% CPU）：
  while (running) {
      n = rte_eth_rx_burst();   // 非阻塞，立刻返回
      if (n > 0)
          process(n);
      // 不睡眠！继续轮询
  }
  ↑ 线程 100% CPU 占用，但延迟最低（纳秒级响应）
```

| 模型 | 延迟 | CPU 占用 | 适用场景 |
|------|------|----------|----------|
| 阻塞/睡眠 | 微秒级（唤醒开销） | 低 | 一般应用 |
| 轮询 (polling) | 纳秒级 | 100% | HFT / DPDK |

> **HFT 核心取舍**：用 100% CPU 换取最低延迟。每个 lcore 线程独占一个 CPU 核心，不睡眠，
> 持续轮询网卡/队列。看起来"浪费" CPU，但避免了上下文切换和唤醒延迟。

## HFT / DPDK 关联总结

| 概念 | HFT / DPDK 应用 |
|------|-----------------|
| **`_Thread_local`** | 每 lcore 独立数据（统计计数器、本地缓存） |
| **线程绑核** | 每 lcore 绑定一个 CPU 核心（`pthread_setaffinity_np`） |
| **轮询模型** | 100% CPU 持续轮询，不睡眠，最低延迟 |
| **无锁数据结构** | `rte_ring` 连接各 lcore（不用 mutex，见 Ch21） |
| **mutex 只在初始化** | 热路径完全不用锁 |
| **条件变量不用** | 轮询代替等待 |

## 自测题

<details><summary>1. DPDK 为什么用轮询而不是阻塞/条件变量？</summary>

阻塞和条件变量有上下文切换开销——线程睡眠后被唤醒需要微秒级延迟（内核调度、cache miss）。
HFT 要求纳秒级响应，轮询模式下线程始终运行（100% CPU），数据到达后立刻处理，没有唤醒延迟。
代价是 CPU 占用率高，但对 HFT 来说延迟比 CPU 利用率重要。
</details>

<details><summary>2. <code>_Thread_local</code> 和全局变量有什么区别？为什么 HFT 用它？</summary>

全局变量所有线程共享——修改需要同步（锁或原子操作），有 contention 和 cache 伪共享问题。
`_Thread_local` 每线程独立一份——互不干扰，无需同步，无 contention。HFT 中每 lcore 的统计
计数器用 `_Thread_local`：每个线程只更新自己的副本，汇总时才读取所有线程的值（无锁读取，
因为运行时各 lcore 只写自己的）。内核的 `DEFINE_PER_CPU` 是同一概念。
</details>

<details><summary>3. 为什么 <code>cnd_wait</code> 要在 <code>while</code> 循环中？</summary>

防止虚假唤醒 (spurious wakeup)——`cnd_wait` 可能在没有 `cnd_signal` 的情况下返回
（POSIX 允许实现这样）。如果在 `if` 中，虚假唤醒后不会重新检查条件，可能处理了未准备好的数据。
`while` 循环确保每次唤醒后都重新检查条件，只有条件为真才继续执行。
</details>

<details><summary>4. HFT 热路径为什么不用 mutex？用什么替代？</summary>

mutex 有上下文切换开销（竞争时）、不确定延迟（等锁时间取决于其它线程）、cache 伪共享问题。
HFT 热路径替代方案：① 每 lcore 独立数据（`_Thread_local`）——完全不共享；
② 无锁队列（`_Atomic` + 内存序）——共享但无锁，见 Ch21；
③ 批量处理——减少队列操作频率，amortize 同步成本。
mutex 只在初始化/配置阶段使用，运行时完全不碰锁。
</details>

<details><summary>5. C11 <code>threads.h</code> 和 <code>pthread</code> 在 HFT 中怎么选？</summary>

学习/模型理解用 `threads.h`（C 标准，概念清晰）；生产环境用 `pthread`（功能全：CPU 亲和性、
实时调度、信号掩码、futex）。DPDK 实际用 pthread + `pthread_setaffinity_np` 做绑核。
`threads.h` 不支持绑核和调度策略，这是 HFT 的硬需求，所以生产只能用 pthread。
</details>
