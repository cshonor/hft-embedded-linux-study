## 5.6 内存一致性模型

### Coherence vs Consistency

| 概念 | 回答问题 |
|------|----------|
| **Coherence（一致性）** | 对 **同一地址**，各核看到的值是否一致？ |
| **Consistency（一致性模型）** | 对 **不同地址**，读写 **以何种顺序** 被其他核观察到？ |

Coherence 不规定：`A` 写完立刻能否看到 `B` 的写 — 那是 Consistency。

---

### 顺序一致性 (Sequential Consistency, SC)

**定义直觉：** 所有核的操作结果，如同某种 **全局交错顺序**，且每核程序顺序保持。

- 最直观、最易推理
- 硬件 **性能代价高** — 很少作为实际实现

---

### 宽松模型 (Relaxed Models)

允许 **乱序执行/缓冲** 以提升性能：

| 模型 | 要点 |
|------|------|
| **TSO** (Total Store Order) | x86 近似行为；写可缓冲，读不能越过未决写（简化） |
| **PSO** | 部分存储序 |
| **Weak ordering** | 更弱 |
| **Release Consistency** | **获取 (acquire)** / **释放 (release)** 语义划分同步点 |

**关键保证：** **无数据竞争 (data-race-free)** 的同步程序，在宽松模型下仍可正确 — 用锁/原子建立 **happens-before**。

| HFT 视角 |
|----------|
| C++ `memory_order_acquire/release`、Java `volatile` — 映射到 **释放一致性** |
| **无锁结构** 必须显式序：SPSC ring buffer 的 **publish 顺序**（写数据 → release store 索引） |
| **错误用 `relaxed` 读标志** → 看到半初始化对象 — 极难复现 bug |
| x86 对程序员较「友好」(TSO)，**ARM 更弱** — 跨平台代码不能假设 TSO |
| Store buffer 导致 **写后读仍见旧值** — 理解 [Ch3 ROB](../chapter-03-instruction-level-parallelism/notes/section-3.6-硬件推测与ROB.md) 与 **内存序** 的硬件根因 |

→ [01-CSAPP Ch12 §12.7](../../../02-computer-systems/chapter-12-concurrent-programming/)

---
