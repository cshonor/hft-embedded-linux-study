## ⑨ 每个 CPU 的分配 · Per-CPU

**SMP 优化** — 每处理器 **独立变量副本**：

| 收益 | 说明 |
|------|------|
| **少锁** | 本 CPU 通常只写自己的副本 |
| **少 cache thrashing** | 避免多核抢同一缓存行 |

| 接口（2.6+） | `percpu` 宏族 — 声明、分配、`__get_cpu_var` 等 |

→ **Ch 8** softirq per-CPU · **Ch 10** `preempt_disable` + per-CPU 数据

**HFT：** 用户态 **每线程/每核一条队列**（false sharing 意识）与内核 per-CPU 同构。

---
