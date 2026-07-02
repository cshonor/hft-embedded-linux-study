## 3. 指令级并发 (Instruction Concurrency)

> **单核内** 并行 — 超标量 + 乱序执行

---

### 一、超标量与乱序

现代 CPU **几乎均为超标量**：

- **无依赖** 的指令可 **乱序** 执行  
- **单周期** 可 **派发/完成** 多条微操作（µop）  

**例（书载）：** Haswell 单周期最多约 **8 条 µop** 派发 — **IPC（每周期指令数）** 可 >1。

---

### 二、对 DPDK 的含义

| 友好代码 | 不友好代码 |
|----------|------------|
| **少分支**、可预测路径 | 复杂分支、指针 chasing |
| **指令独立** — Load/Store 与算术交错 | 长 **依赖链** — 后指令等前指令 |
| 配合 **prefetch**（Ch2） | 频繁 **stall** 等内存 |

**热路径：** 帮助 CPU **填满流水线** — 与 [section-4 SIMD](./section-4-数据并行与SIMD.md) `rte_memcpy` 双 Load 策略一致。

→ [02-Hennessy 流水线](../../../02-Computer-Architecture-6th/chapter-02-memory-hierarchy-design/)

---

← [2. 多核扩展](./section-2-多核性能与可扩展性.md) · 下一节 [4. SIMD](./section-4-数据并行与SIMD.md)
