## 4. Cache 一致性与 DPDK 无锁化设计

> 多核 **同时读写** 同一内存 → **Cache 一致性** 开销

---

### 一、MESI 协议（概念）

多核维护 Cache 一致性的典型四状态：

| 状态 | 含义 | 可读? | 可写? | 其他核有副本? |
|------|------|:---:|:---:|:---:|
| **M** Modified | 本核独占且已改 | ✅ | ✅ | ❌ |
| **E** Exclusive | 本核独占、未改 | ✅ | ✅ (→M) | ❌ |
| **S** Shared | 多核共享只读 | ✅ | ❌ (需先 invalidate) | ✅ |
| **I** Invalid | 无效 | ❌ | ❌ | — |

**跨核写同一 Cache Line** → 总线发出 **Invalidate** 消息 → 其他核 **丢弃该行** → 写者获得 **M 状态** — 这个过程涉及 **跨核通信**，延迟可达 **40-100 cycle**。

**MESI 状态转换（简化）：**

```
         读 miss (BusRd)
    I ─────────────────→ S (有其他副本) 或 E (无其他副本)

    E ──写──→ M         S ──写──→ 发 BusRfo → 其他核 invalidate → M

    M ──其他核读 (BusRd)──→ S (先写回内存)
    S ──其他核写 (BusRfo)──→ I
```

 [15-computer-architecture 一致性](../../../../15-computer-architecture/chapter-02-memory-hierarchy-design/)

---

### 二、伪共享 (False Sharing)

**不同变量** 落在 **同一 64B Cache Line**：

- 核 A 写 `counter_a`，核 B 写 `counter_b`
- 每次写都触发 MESI invalidate → 行在两核间 **乒乓** → 性能 **暴跌**

**性能影响示例：**

```c
/* ❌ False Sharing — 两个计数器在同一 cache line */
struct bad_stats {
    uint64_t rx_pkts;   /* core 0 写 */
    uint64_t tx_pkts;   /* core 1 写 */
};  /* 16B < 64B → 同一 cache line */

/* ✅ 修复 — padding 到 cache line */
struct good_stats {
    uint64_t rx_pkts;   /* core 0 独占行 */
    char pad[56];       /* 填充到 64B */
    uint64_t tx_pkts;   /* core 1 独占行 */
};

/* ✅ 更好的方式 — DPDK 宏 */
struct dpdk_stats {
    uint64_t rx_pkts __rte_cache_aligned;
    uint64_t tx_pkts __rte_cache_aligned;
};
```

---

### 三、DPDK 最佳实践

| 手段 | 说明 | 效果 |
|------|------|------|
| **Cache Line 对齐** | **`__rte_cache_aligned`** — 结构体 **64B 对齐**，热点字段 **独占行** | 消除 false sharing |
| **Per-core 资源** | 每 lcore **独立** 统计、队列、mempool cache — **避免跨核写同一变量** | 消除跨核 MESI 流量 |
| **专属 RX/TX 队列** | 网卡 **多队列** — 一核一队，减少锁与一致性流量 | 无锁收发路径 |
| **Read-Mostly 分离** | 只读配置 vs 可写计数器 **分结构体** | 读路径不碰写行 |

**Per-lcore 统计模式：**

```c
/* DPDK 推荐的 per-core 统计结构 */
struct lcore_stats {
    uint64_t rx_packets  __rte_cache_aligned;
    uint64_t tx_packets  __rte_cache_aligned;
    uint64_t rx_bytes    __rte_cache_aligned;
    uint64_t tx_bytes    __rte_cache_aligned;
    uint64_t dropped     __rte_cache_aligned;
} __rte_cache_aligned;

/* 每个 lcore 一份独立实例 — 无共享 */
static struct lcore_stats stats[RTE_MAX_LCORE];

/* 读取时聚合（非热路径） */
uint64_t total_rx = 0;
for (int i = 0; i < RTE_MAX_LCORE; i++)
    total_rx += stats[i].rx_packets;
```

 [Ch1 方法论 · 水平扩展](../../chapter-01-dpdk-intro/notes/section-4-底层方法论.md) · [14 HFT 无锁环](../../../../14-hft-engineering/chapter-07-lockless-data-structures-memory-layout/)

**原则：** 不是「少锁」，而是 **从设计上不共享可写 cache line**。

---

← [3. 预取](./section-3-Cache预取.md) · 下一节 [5. 大页](./section-5-大页Hugepages.md)
