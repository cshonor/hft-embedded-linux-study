## 2.1 引言与存储器层次

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

---
