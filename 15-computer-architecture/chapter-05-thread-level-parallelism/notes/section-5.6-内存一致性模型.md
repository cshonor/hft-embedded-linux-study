## 5.6 内存一致性模型


> ↔ [CSAPP §12.7 并发问题](../../../02-computer-systems/chapter-12-concurrent-programming/notes/section-12.7-其他并发问题.md)

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
| Store buffer 导致 **写后读仍见旧值** — 理解 [Ch3 ROB](../../chapter-03-instruction-level-parallelism/notes/section-3.6-硬件推测与ROB.md) 与 **内存序** 的硬件根因 |

→ [02-CSAPP Ch12 §12.7](../../../02-computer-systems/chapter-12-concurrent-programming/)


### 常见陷阱

- 在宽松模型上用 `memory_order_relaxed` 读标志 — relaxed 不保证看到 **写数据 → 写标志** 的顺序 → 可能读到 **半初始化对象** → 极难复现 bug
- x86 代码直接移植到 ARM 不调整内存序 — x86 是 TSO（较强），ARM 是 **弱模型**；x86 上「碰巧正确」的代码在 ARM 上可能 data race
- 混淆 Coherence 和 Consistency — Coherence 管单地址值一致；Consistency 管多地址 **观察顺序**；一致性协议不规定跨地址顺序

### 自测题（点击展开）

<details>
<summary>Q1. Coherence 和 Consistency 的区别是什么？各回答什么问题？</summary>

Coherence：对 **同一地址**，各核看到的值是否一致？（单地址值一致）Consistency：对 **不同地址**，读写以何种 **顺序** 被其他核观察到？（跨地址序）Coherence 不规定 A 写完后何时能看到 B 的写。

</details>

<details>
<summary>Q2. TSO（Total Store Order）是什么？x86 为什么对程序员较「友好」？</summary>

TSO：写可缓冲（store buffer），但 **读不能越过未决写**（同地址读看到最新写）。x86 近似 TSO → 程序员不需要太多显式 barrier → 更容易写正确并发代码。ARM 更弱 → 需要 `dmb`/`acquire-release`。

</details>

<details>
<summary>Q3. SPSC ring buffer 的 publish 顺序为什么重要？用 C++ atomic 怎么写？</summary>

写数据 → release store 索引 → 消费者 acquire load 索引 → 看到索引后才读数据。若用 `relaxed` → 可能 **先更新索引再写数据** → 消费者看到索引但读到旧数据。正确写法：`data[idx] = val; head.store(idx+1, memory_order_release);` / `h = head.load(memory_order_acquire); if (h > tail) { val = data[tail]; }`

</details>

<details>
<summary>Q4. 为什么 x86 代码直接移植到 ARM 可能出 bug？</summary>

x86 TSO 较强 → 很多「本该用 barrier」的代码 **碰巧正确**。ARM 弱模型 → 没有 barrier 时重排更激进 → data race 暴露。移植时必须审查所有 **跨线程共享访问** 的内存序，用 `acquire/release` 或 `seq_cst` 显式标注。

</details>
---
