## 4. Cache 一致性与 DPDK 无锁化设计

> 多核 **同时读写** 同一内存 → **Cache 一致性** 开销

---

### 一、MESI 协议（概念）

多核维护 Cache 一致性的典型四状态：

| 状态 | 含义 |
|------|------|
| **M** Modified | 本核独占且已改 |
| **E** Exclusive | 本核独占、未改 |
| **S** Shared | 多核共享只读 |
| **I** Invalid | 无效 |

**跨核写同一 Cache Line** → 总线 **invalidate / 同步** — **极慢**。

→ [02-Hennessy 一致性](../../../02-Computer-Architecture-6th/chapter-02-memory-hierarchy-design/)

---

### 二、伪共享 (False Sharing)

**不同变量** 落在 **同一 64B Cache Line**：

- 核 A 写 `counter_a`，核 B 写 `counter_b`  
- 硬件仍视为 **同一行争用** → 性能 **暴跌**

---

### 三、DPDK 最佳实践

| 手段 | 说明 |
|------|------|
| **Cache Line 对齐** | **`__rte_cache_aligned`** — 结构体 **64B 对齐**，热点字段 **独占行** |
| **Per-core 资源** | 每 lcore **独立** 统计、队列、 mempool 消费 — **避免跨核写同一变量** |
| **专属 RX/TX 队列** | 网卡 **多队列** — 一核一队，减少锁与一致性流量 |

→ [Ch1 方法论 · 水平扩展](../chapter-01-dpdk-intro/notes/section-4-底层方法论.md) · [15 HFT 无锁环](../../../15-HFT-Low-Latency-Practice/)

**原则：** 不是「少锁」，而是 **从设计上不共享可写 cache line**。

---

← [3. 预取](./section-3-Cache预取.md) · 下一节 [5. 大页](./section-5-大页Hugepages.md)
