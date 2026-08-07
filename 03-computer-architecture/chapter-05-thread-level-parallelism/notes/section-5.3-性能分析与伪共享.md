## 5.3 对称共享内存多处理器的性能分析


> ↔ [CSAPP §12.4 共享变量](../../../02-computer-systems/chapter-12-concurrent-programming/notes/section-12.4-多线程程序中的共享变量.md) · [Harris §8.2 存储器性能分析](../../../00-digital-logic-cpu/ch08_memory/8.2_存储器系统性能分析.md)

### 一致性流量来源

| 类型 | 定义 | 是否必要 |
|------|------|----------|
| **真共享 (True sharing)** | 多核 **确实** 读写 **同一变量** | 是 — 程序语义需要 |
| **伪共享 (False sharing)** | 多核写 **不同变量**，但落在 **同一 cache block** | 否 — 协议误伤 |

**伪共享机制：** 写失效以 **cache line（通常 64B）** 为粒度 — 他核上同 line 内任意数据一并失效。

---

### 性能影响因素

| 因素 | 影响 |
|------|------|
| **缓存容量** | 工作集大 → 更多容量失效 |
| **处理器数量** | 核越多，共享与一致性流量越大 |
| **缓存块大小** | 块越大，伪共享「误伤」范围越大 |
| **工作负载** | OLTP 锁/记录竞争 vs 多道程序 OS 干扰 |

| HFT 视角 |
|----------|
| **经典坑**：`struct { atomic<uint64_t> c1; atomic<uint64_t> c2; }` 两核各写 c1/c2 → 同 line **乒乓** |
| **对策**：`alignas(64)` 分 line；**每核/per-thread 私有** 计数器，周期合并 |
| 订单簿：热写结构 **按核分片** 或单写者 — 避免多写者共享 line |
| `perf c2c`（cache-to-cache）可看 **伪共享热点**（需较新内核/工具） |

→ [Ch2 §2.3](../../chapter-02-memory-hierarchy-design/notes/section-2.3-缓存性能十项高级优化.md)


### 常见陷阱

- 只关注 true sharing 而忽略 false sharing — true sharing 是语义必要的；false sharing 是 **协议误伤**，可通过 padding 消除
- 用 `alignas(64)` padding 后不做实测 — 某些编译器/ABI 可能不在结构体间插入 padding；需 `perf c2c` 或 vtune 确认 false sharing 已消除
- 以为只读共享不会触发一致性流量 — 正确，但 **写一个字段就会使整 line 在其他核失效**；混在热结构里的冷写也会污染只读字段

### 自测题（点击展开）

<details>
<summary>Q1. 什么是 false sharing？它和 true sharing 的区别是什么？</summary>

True sharing：多核读写 **同一变量** — 语义需要的一致性流量。False sharing：多核写 **不同变量** 但在同一 cache line → MESI 以 line 为粒度失效 → **不必要的乒乓**。False sharing 是纯性能损失，可消除。

</details>

<details>
<summary>Q2. 给出 false sharing 的代码示例和 3 种对策。</summary>

示例：`struct { atomic<u64> c1; atomic<u64> c2; }` 两核各写 c1/c2 → 同 line 乒乓。
对策：1) `alignas(64)` 分 line 2) **per-thread 私有计数器**，周期合并 3) 热写结构 **按核分片** 或单写者。

</details>

<details>
<summary>Q3. 影响 SMP 性能的 4 个因素是什么？cache 块大小如何影响 false sharing？</summary>

1) 缓存容量（工作集 vs 容量失效）2) 处理器数量（核越多一致性流量越大）3) **缓存块大小**（块越大 false sharing「误伤」范围越大）4) 工作负载（锁竞争 vs OS 干扰）。64B line → 8 个 u64 可能同 line → false sharing 风险高。

</details>

<details>
<summary>Q4. `perf c2c` 是什么？HFT 中怎么用它？</summary>

`perf c2c` = cache-to-cache 工具，检测 **false sharing 热点** — 哪些 cache line 在核间乒乓最频繁。用法：`perf c2c record ./my_app` → `perf c2c report`。看 HITM（Hit Modified）计数高的 line → 定位 false sharing 位置。

</details>
---
