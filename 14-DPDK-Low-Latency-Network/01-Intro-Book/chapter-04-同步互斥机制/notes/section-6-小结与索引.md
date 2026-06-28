## 6. 小结与后续索引

---

### 一、本章总结

**四种机制 · 扬长避短：**

| 机制 | 适用 | 避免 |
|------|------|------|
| **原子操作** | 单变量计数、位操作、CAS 基元 | 保护大结构 / 长临界区 |
| **自旋锁** | **短**临界区、中断上下文、不可睡眠 | 持锁 I/O、长循环 |
| **读写锁** | **读多写少** — 路由/LPM/ACL/memzone | 写频繁、写路径在 tick 热路径 |
| **无锁 ring** | MP/MC **高速队列**、包/对象传递 | 需严格顺序且无法批量时的复杂结构 |

```
Ch3 并行计算 — 多核扩展的代价（同步、Cache）
    ↓
Ch4 同步互斥（本章）— 原子 / 锁 / 无锁 ring
    ↓
mbuf + mempool + ring — 预分配对象池与核间传递
    ↓
Ch8 流分类与多队列 — 硬件 RSS + 软件分核
```

---

### 二、后续章节索引

| Ch4 主题 | 继续读 |
|----------|--------|
| rte_ring / mbuf | [chapter-02-mbuf与内存池.md](../chapter-02-mbuf与内存池.md) 🔴 |
| 多队列 / 核间分发 | [chapter-08-流分类与多队列](../chapter-08-流分类与多队列/) 🔴 |
| PMD 轮询 | [chapter-03-PMD与轮询模式.md](../chapter-03-PMD与轮询模式.md) 🔴 |
| Cache / 伪共享 | [chapter-02-Cache与内存](../chapter-02-Cache与内存/) 🔴 |
| 内核同步对照 | [ULK Ch5 内核同步](../../../06-Understanding-Linux-Kernel/chapter-05-kernel-synchronization/) |
| 并行 / Amdahl | [chapter-03-并行计算](../chapter-03-并行计算/) 🔴 |
| HFT 锁与队列 | [15 HFT 工程](../../../15-HFT-Low-Latency-Practice/) |

---

← [5. 无锁机制](./section-5-无锁机制.md) · 下一章 [chapter-02-mbuf](../chapter-02-mbuf与内存池.md) · [Ch3 并行](../chapter-03-并行计算/)
