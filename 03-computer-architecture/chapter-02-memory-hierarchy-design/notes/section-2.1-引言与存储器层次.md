## 2.1 引言与存储器层次


> ↔ [CSAPP §6.3 层次结构](../../../02-computer-systems/chapter-06-memory-hierarchy/notes/section-6.3-层次结构与缓存概念.md) · [Harris §8.3 高速缓存](../../../00-digital-logic-cpu/ch08_memory/8.3_高速缓存.md)

### 性能鸿沟

处理器运算速度增长远快于 **主存访问延迟** 的改善 — 「存储墙 / Memory Wall」。架构师用 **层次结构（Hierarchy）** 把昂贵但快的存储与便宜但慢的存储组合起来，靠 **局部性** 让多数访问落在较快层。

### 典型层次（由快到慢）

```
寄存器 → L1 → L2 → L3 → 主存 (DRAM) → Flash/磁盘
```

| 层 | 典型特征 |
|----|----------|
| **L1** | 片上 SRAM，1–4 周期级，分 I/D cache |
| **L2/L3** | 更大、更慢；L3 常多核共享 |
| **DRAM** | 容量大、延迟高（数十～上百 ns） |
| **Flash** | 块擦写、非易失，PMD/PC 主存储 |

### AMAT 公式

\[
\text{AMAT} = T_{\text{hit}} + \text{Miss Rate} \times T_{\text{miss penalty}}
\]

**设计目标：** 降低 \(T_{\text{hit}}\)、降低 Miss Rate、降低 Miss Penalty，或在带宽/功耗约束下权衡。

| HFT 视角 |
|----------|
| 热路径「多一次 L3 miss」≈ **数百周期** — 比算几条指令贵得多 |
| 优化顺序：**减少 miss** > 微优化算术指令（呼应 [Ch1 Amdahl](../../chapter-01-quantitative-design-fundamentals/notes/section-1.9-计算机设计的量化原则.md)） |
| `perf stat` 看 `cache-misses`、`LLC-load-misses` — 量化 AMAT 各分量 |

→ 程序员视角：[01-CSAPP Ch6](../../../02-computer-systems/chapter-06-memory-hierarchy/)


### 常见陷阱

- 把 AMAT 公式当唯一指标 — AMAT 只反映平均延迟，HFT 热路径看的是 **尾部延迟**（P99 miss penalty），一次 L3 miss 就可能毁掉整个 tick 预算
- 认为「L1 命中率 99% 就够了」— 1% 的 L2/L3 miss 贡献了绝大部分时间（AMAT 中 miss rate × penalty 主导），看 miss 次数而非命中率
- 忽略 DRAM 延迟几乎不降的事实 — DDR 带宽翻倍但随机访问延迟仍在 ~100ns，用顺序带宽推测随机延迟会严重高估性能

### 自测题（点击展开）

<details>
<summary>Q1. AMAT = 4 周期（L1 hit）+ 5% × 100 周期（L2 miss penalty）。AMAT 是多少？哪一项主导？</summary>

AMAT = 4 + 0.05 × 100 = 9 周期。L1 命中贡献 4，miss 贡献 5 — **miss 主导**，尽管命中率 95%。这说明降低 miss rate 比降低 hit time 更有效。

</details>

<details>
<summary>Q2. HFT 热路径上，一次 LLC miss ≈ 40-100ns。在 3GHz CPU 上等价于多少周期？对 tick 预算意味着什么？</summary>

40-100ns ≈ 120-300 周期。如果 tick 预算是 1μs（3000 周期），一次 LLC miss 就吃掉 4-10% 预算。多次 miss 可直接导致超时。

</details>

<details>
<summary>Q3. 存储墙（Memory Wall）是什么？为什么 DRAM 带宽增长远快于延迟下降？</summary>

存储墙指 CPU 速度增长远快于 DRAM 延迟改善。带宽通过多 Bank 交错、更宽接口（DDR→DDR5）提升，但 **随机访问延迟受物理限制**（行激活、列访问的 RC 延迟）几乎不降。→ 局部性愈发重要。

</details>
---
