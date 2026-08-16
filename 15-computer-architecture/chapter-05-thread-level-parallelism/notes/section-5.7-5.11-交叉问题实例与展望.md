## 5.7–5.11 交叉领域问题、实例与展望


> ↔ [CSAPP §12.8 小结](../../../02-computer-systems/chapter-12-concurrent-programming/notes/section-12.8-小结.md)

### 5.7 多层次缓存的包含性 (Inclusion)

| 概念 | 说明 |
|------|------|
| **Inclusive** | L3 **包含** L1/L2 内容副本 — 简化一致性、目录 |
| **优势** | 监听/目录可查 L3；换出时易维护 |
| **代价** | L3 容量压力、部分设计 **非包含** 换容量 |

| HFT 视角 |
|----------|
| LLC 争用：多线程工作集之和超 L3 → **互挤失效** — 绑核隔离 workload |

---

### 5.8–5.10 真实多核对比与 SMT

书中对比 **IBM Power8、Intel Xeon E7、Fujitsu SPARC64 X+** 等服务器芯片：

| 维度 | 差异 |
|------|------|
| 核数、SMT、缓存层次 | 影响 Java/PARSEC 加速比与能效 |
| **SMT + 多核** | 吞吐升，单线程延迟 **不一定** 改善 |

| HFT 视角 |
|----------|
| 与 [Ch3 SMT](../../chapter-03-instruction-level-parallelism/notes/section-3.11-多线程技术.md) 一致：**关键路径关 HT、独占物理核** |
| 选型看 **L3 容量、内存通道、NUMA 拓扑** — 不止核数 |

---

### 5.11 多核扩展的未来与谬误

| 限制 | 含义 |
|------|------|
| **功耗墙 / 暗硅** | 不能所有晶体管同时全速 — 加核 ≠ 线性加速 |
| **Amdahl** | 串行段封顶 |
| **方向** | **DSA**、云规模、专用加速器（→ [Ch7](../../chapter-07-domain-specific-architectures/)） |

**常见谬误：**

| 谬误 | 真相 |
|------|------|
| 核越多越好 | 受并行度、一致性、功耗约束 |
| 加核不改软件 | 伪共享、锁、NUMA 若不调，性能 **倒退** |
| SC 是硬件默认 | 实际是 **宽松模型 + 同步原语** |

---

### 本章小结

Ch5 链路：

```
NUMA/SMP → MESI 监听 / 目录 → 真共享 vs 伪共享
→ 原子原语 / 自旋锁 → 内存序 (release/acquire)
→ 多核扩展极限
```

**HFT 行动清单：**

1. `numactl` 绑定内存与 CPU  
2. 审查共享写结构的 **cache line 布局**  
3. 无锁通道用 **正确 memory order**  
4. 隔离 housekeeping 与 hot 线程（`isolcpus`）  


### 常见陷阱

- 以为 SMT（超线程）对单线程延迟有帮助 — SMT 提升的是 **吞吐**，不是单线程延迟；HFT 关键路径应 **关 HT、独占物理核**
- 选 CPU 只看核数 — 还需看 **L3 容量、内存通道数、NUMA 拓扑**；L3 太小 → 多策略互挤；内存通道少 → 带宽瓶颈
- 以为加核不改软件就行 — 伪共享、锁竞争、NUMA 访存模式若不调，加核后性能可能 **倒退**

### 自测题（点击展开）

<details>
<summary>Q1. Inclusive 和 Non-inclusive cache 层次各有什么优劣？HFT 关注什么？</summary>

Inclusive：L3 包含 L1/L2 副本 → 简化一致性/目录查找，但 **L3 容量被 L1/L2 副本占用**。Non-inclusive：L3 不含 L1/L2 → 容量更大，但一致性维护更复杂。HFT 关注 L3 争用 → inclusive 时多线程工作集之和超 L3 → 互挤失效。

</details>

<details>
<summary>Q2. 为什么 HFT 关键路径要关超线程（HT/SMT）？怎么关？</summary>

SMT 两个线程共享物理核的执行资源（ALU/LSU/cache）→ 兄弟线程 **争用资源** → 单线程延迟不稳定。关 HT：BIOS 设置或 `echo 0 > /sys/devices/system/cpu/cpuN/online`（关偶数核的对线程）。关键路径 **独占物理核**。

</details>

<details>
<summary>Q3. 「加核不改软件，性能倒退」的 3 个原因是什么？</summary>

1) **伪共享** — 多核写同 line → 乒乓 → 比单核更慢 2) **锁竞争** — 串行段不变，更多核等锁 → Amdahl 限制 3) **NUMA** — 跨 socket 访存 → 延迟增大。→ 加核前先调数据布局、锁粒度、NUMA 绑定。

</details>

<details>
<summary>Q4. 多核扩展的 3 个硬限制是什么？</summary>

1) **功耗墙/暗硅** — 不能所有晶体管同时全速 → 加核 ≠ 线性加速 2) **Amdahl** — 串行段封顶 3) **一致性/通信开销** — 核越多一致性流量越大 → 扩展性递减。方向：DSA、专用加速器。

</details>
---
