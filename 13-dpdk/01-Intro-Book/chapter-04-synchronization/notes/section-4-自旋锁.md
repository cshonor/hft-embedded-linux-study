## 4. 自旋锁 (Spinlocks)

---

### 一、忙等待 vs 睡眠

| | **自旋锁** | **互斥锁 (mutex)** |
|---|-----------|-------------------|
| 资源被占用时 | **循环查询（自旋）** 等待 | 线程 **睡眠**，唤醒后竞争 |
| 上下文切换 | **无**（等待期间） | 有 — 代价高（~1-5μs） |
| 中断上下文 | **可用** | 通常 **不可用**（不可睡眠） |
| 临界区长度 | 必须 **极短**（<100 cycle） | 可较长 |
| CPU 占用 | 100%（自旋时） | 0%（睡眠时） |

DPDK 热路径追求 **低延迟** — 短临界区用自旋锁避免 **睡眠/唤醒** 开销。

---

### 二、DPDK：`rte_spinlock_t`

```c
/* rte_spinlock 底层实现 — x86 */
typedef struct {
    volatile int locked;  /* 0=未锁, 1=已锁 */
} rte_spinlock_t;

/* lock — 自旋直到获得 */
static inline void
rte_spinlock_lock(rte_spinlock_t *sl)
{
    /* 使用 xchg 指令 — 自带 LOCK 前缀，原子交换 */
    while (__sync_lock_test_and_set(&sl->locked, 1)) {
        /* 自旋等待 — 使用 pause 指令降低功耗和流水线争用 */
        while (sl->locked)
            rte_pause();  /* _mm_pause() — 提示 CPU 这是自旋等待 */
    }
}

/* unlock — 简单写入 0（带 release 语义） */
static inline void
rte_spinlock_unlock(rte_spinlock_t *sl)
{
    __sync_lock_release(&sl->locked);
}
```

**`rte_pause()` 的作用：**
- x86 `_mm_pause()` — 提示 CPU 处于自旋等待循环
- 降低功耗（避免超标量疯狂预取）
- 减少 MESI 监听流量（减少对锁变量的 cache line 争用）
- ~140 cycle 延迟，给其他核更多机会释放锁

---

### 三、应用场景

| 模块 | 典型用途 |
|------|----------|
| **告警 / 日志** | 多核写日志缓冲区 |
| **中断机制** | 与 PMD 中断路径共享状态 |
| **内存共享** | 全局结构短临界区 |
| **link bonding** | 聚合端口状态更新 |
| **配置更新** | 运行时修改转发表（读多写少 → 考虑 rwlock） |

```c
/* DPDK spinlock 使用模式 */
static rte_spinlock_t stats_lock = RTE_SPINLOCK_INITIALIZER;

/* 热路径 — 临界区极短 */
rte_spinlock_lock(&stats_lock);
global_counter++;        /* 仅 1-2 条指令 */
rte_spinlock_unlock(&stats_lock);
```

---

### 四、风险与调优

| 风险 | 说明 | 对策 |
|------|------|------|
| **CPU 空转** | 持锁时间长 → 其他核 **burn cycles** | 临界区 < 100 cycle |
| **优先级反转** | 低优先级持锁阻塞高优先级 lcore | 避免热路径用锁 |
| **Cache 行 bouncing** | 锁变量在多核间 **频繁 invalidate** | per-lcore 数据优先 |
| **活锁** | CAS 竞争激烈时所有核都在重试 | 改用 SP/SC 队列避免共享 |

**HFT：** 热路径 **尽量避免 spinlock**；若必须用 — 临界区仅几条指令，且 **per-lcore 数据** 优先。

```c
/* HFT 推荐 — per-lcore 统计替代全局 spinlock */
struct lcore_stats {
    uint64_t rx_pkts __rte_cache_aligned;
    uint64_t tx_pkts __rte_cache_aligned;
    uint64_t dropped __rte_cache_aligned;
} __rte_cache_aligned;

static struct lcore_stats stats[RTE_MAX_LCORE];

/* 热路径 — 零锁 */
stats[rte_lcore_id()].rx_pkts++;

/* 非热路径 — 聚合读取 */
uint64_t total = 0;
for (int i = 0; i < RTE_MAX_LCORE; i++)
    total += stats[i].rx_pkts;
```

 [ULK Ch5 §4 自旋锁](../../../../16-linux-kernel-deep/chapter-05-kernel-synchronization/notes/section-4-自旋锁.md) · [14 HFT 无锁](../../../../14-hft-engineering/chapter-07-lockless-data-structures-memory-layout/)

---

← [3. 读写锁](./section-3-读写锁.md) · 下一节 [5. 无锁机制](./section-5-无锁机制.md)
