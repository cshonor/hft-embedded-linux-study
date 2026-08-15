## 2.3 缓存性能十项高级优化


> ↔ [CSAPP §6.4.7 Cache参数影响](../../../02-computer-systems/chapter-06-memory-hierarchy/notes/section-6.4.7-Cache参数的性能影响.md) · [CSAPP §6.5 缓存友好代码](../../../02-computer-systems/chapter-06-memory-hierarchy/notes/section-6.5-编写高速缓存友好的代码.md) · [Harris §8.2 存储器性能分析](../../../00-digital-logic-cpu/ch08_memory/8.2_存储器系统性能分析.md)

原书按对 **命中时间、未命中率、未命中惩罚、带宽、功耗** 的影响，将十项优化分为五类。本章核心之一。

---

### A. 缩短命中时间

#### 1. 小而简单的 L1

| 做法 | 效果 |
|------|------|
| L1 容量适中、结构简单 | **降低命中时间**、**降低功耗** |
| 复杂度高的大 L1 | 命中变慢、能耗上升 |

#### 2. 路预测 (Way prediction)

在 **组相联** cache 中 **预测** 下一次访问的「路」，只对预测路做 tag 比较 → 缩短命中路径、省电。

| HFT 视角 |
|----------|
| 你控制不了微架构，但知道：**关联度与容量** 影响命中延迟 — 热数据结构宜 **对齐、紧凑、可预测访问模式** |

---

### B. 提升缓存带宽

#### 3. 流水线化与多 Bank 缓存

支持 **每周期多次独立访问**（不同 bank 并行）。

#### 4. 非阻塞缓存 (Nonblocking cache)

**Hit under miss** — 未命中处理进行时，仍可服务后续 **命中** 请求。对 **乱序执行** CPU 关键。

| HFT 视角 |
|----------|
| 多线程同时打 L1/L2 — **false sharing** 会让「逻辑独立」的核争抢同一 cache line（→ [Ch5](../../../../chapter-05-thread-level-parallelism/)） |
| 单线程热循环：miss 时 CPU 仍可能 **乱序执行不依赖该数据的指令** — 但依赖链会 stall |

---

### C. 降低未命中惩罚

#### 5. 关键字优先与提前重启 (Critical word first / early restart)

未命中时 **优先返回 CPU 急需的字**，不必等整 cache line 填满再交给流水线。

#### 6. 合并写缓冲区 (Merging write buffer)

多个写合并为 **一次总线事务** 写回内存，提高总线效率。

| HFT 视角 |
|----------|
| 写密集结构（日志、统计计数器）易触发 **写缓冲与 cache 一致性流量** — 热路径少写共享 cache line |

---

### D. 降低未命中率（软件）

#### 7. 编译器优化：改善局部性

| 技术 | 作用 |
|------|------|
| **循环交换 (Loop interchange)** | 让内层循环 stride-1，提高 **空间局部性** |
| **分块 (Blocking/Tiling)** | 子矩阵/子数组适配 cache 容量，提高 **时间局部性** |

**无需改硬件** 即可显著降 miss rate。

| HFT 视角 |
|----------|
| 订单簿遍历、矩阵化风控：**SoA、分块、热冷分离** |
| **Cache line padding**（通常 64B）：两核各写相邻字段 → false sharing → 性能暴跌 |
| 示意：热字段独占 cache line，冷字段另放或 `alignas(64)` 填充 |

---

### E. 并行机制：预取与 HBM 扩展

#### 8. 硬件预取

CPU **预测** 即将访问的地址，提前拉入 cache。

#### 9. 编译器控制预取

插入 `prefetch` 类提示/指令，由程序员或编译器 **显式** 提前请求。

#### 10. HBM 扩展层次（作 L4）

堆叠内存作 **超大 L4**，挑战在于 **tag 开销** 与层次管理。

| HFT 视角 |
|----------|
| 硬件预取对 **顺序扫描** 友好；对 **指针跳跃、哈希表** 可能 **预取污染**（拉入无用 line，挤掉热数据） |
| 显式 `__builtin_prefetch` 仅在对 **访问模式极稳** 的热循环尝试；务必 **profile** |
| 理解 i7 类 CPU「激进预取」— 非常规访问模式反而变慢（见 2.6） |

---

### 十项优化速查表

| # | 优化 | 主要改善 |
|---|------|----------|
| 1 | 小而简单 L1 | 命中时间、功耗 |
| 2 | 路预测 | 命中时间、功耗 |
| 3 | 流水线/多 Bank | 带宽 |
| 4 | 非阻塞缓存 | 带宽、ILP 利用 |
| 5 | 关键字优先 | 未命中惩罚 |
| 6 | 合并写缓冲 | 未命中惩罚、总线 |
| 7 | 编译器分块/交换 | 未命中率 |
| 8 | 硬件预取 | 未命中率 |
| 9 | 编译器预取 | 未命中率 |
| 10 | HBM 作 L4 | 带宽、惩罚、层次扩展 |


### 常见陷阱

- 认为硬件预取总是有益 — 对 **指针追逐、哈希表** 等不可预测访问，预取拉入无用 cache line → **预取污染**，挤掉热数据反而变慢
- 对 false sharing 只做 alignas(64) 就放心了 — 还需确保 **不同核不写同一 padded 结构的不同字段**；per-thread 私有计数器 + 周期合并才是正解
- 分块（Tiling）大小不匹配 cache 容量 — 分块太大 → 工作集溢出 L1/L2 → miss 暴涨；必须用 `perf stat` 实测确定分块大小

### 自测题（点击展开）

<details>
<summary>Q1. 十项优化中，哪些是 **软件可控** 的？哪些是纯硬件的？</summary>

软件可控：#7 编译器分块/循环交换、#9 编译器预取。其余 8 项（小 L1、路预测、多 Bank、非阻塞、关键字优先、写合并、硬件预取、HBM）由 CPU 微架构决定，程序员只能间接利用（如数据布局适配硬件预取）。

</details>

<details>
<summary>Q2. 什么是 false sharing？给出一个 HFT 场景的具体例子和对策。</summary>

两核各写同一 cache line 内不同变量 → MESI 反复 invalidate → line 在核间乒乓。例：`struct { atomic<u64> order_count; atomic<u64> cancel_count; }` 两核各写一个字段。对策：`alignas(64)` 分 line，或 **per-thread 私有计数器**，周期合并。

</details>

<details>
<summary>Q3. 关键字优先（Critical Word First）为什么对乱序 CPU 重要？没有它会怎样？</summary>

cache miss 时，CPU 急需的 **关键字** 先返回，不等整 line 填满。没有它，CPU 要等整 line（64B）传输完才能继续 → 依赖该数据的指令全部 stall 更久。乱序 CPU 可以在关键字到达后立即执行依赖指令，其余 line 填充与执行并行。

</details>

<details>
<summary>Q4. `__builtin_prefetch` 什么时候有用？什么时候有害？</summary>

有用：访问模式 **极稳定** 的热循环（如预计算链表下一节点）。有害：访问不可预测（哈希表、指针追逐）→ 预取无用 line → 污染 cache → 挤掉热数据。务必 profile 对比。

</details>
---
