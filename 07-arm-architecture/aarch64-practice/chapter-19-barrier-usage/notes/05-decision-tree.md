# §19.5 屏障选择决策树

> **来源：** [Ch19 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

系统化的屏障选择方法：先判断是否需要屏障，再判断同步对象（编译器/CPU 间/DMA/系统寄存器），最后选择最弱的足够屏障。能用 LDAR/STLR 就优先用。

## 核心要点

### 屏障选择决策树

```
需要屏障？
├── 编译器重排？ → barrier()（无硬件开销）
├── CPU 间同步？
│   ├── 只约束 Store？ → smp_wmb()（dmb ishst）
│   ├── 只约束 Load？ → smp_rmb()（dmb ishld）
│   └── 约束全部？ → smp_mb()（dmb ish）
├── CPU ↔ DMA？
│   ├── 只写？ → wmb()（dsb st）
│   ├── 只读？ → rmb()（dsb ld）
│   └── 全部？ → mb()（dsb sy）
├── 系统寄存器/TLB？ → dsb + isb
└── 能用 LDAR/STLR？ → 优先用（比显式屏障高效）
```

### 选择原则

| 原则 | 说明 |
|------|------|
| 选最弱的足够屏障 | ishst 比 ish 轻，ish 比 sy 轻 |
| DMB vs DSB | CPU 间用 DMB，DMA 用 DSB |
| 作用域 | CPU 间用 ish，DMA 用 osh/sy |
| LDAR/STLR 优先 | 比 DMB 高效，CPU 能更精确优化 |
| 编译器屏障 | 硬件屏障不阻止编译器重排 |

## HFT 关联

这个决策树是 HFT 无锁编程的实用工具。HFT 开发者在写无锁代码时，按决策树选择屏障可以避免两个极端：1) 过弱屏障（正确性问题，随机失败）；2) 过强屏障（性能问题，不必要停顿）。HFT 最常用的选择是"能用 LDAR/STLR 就用"——C++ `memory_order_acquire`/`release` 自动编译为 LDAR/STLR，是最优选择。DMA 场景（网卡收发）用 `dsb sy`，CPU 间同步用 `dmb ish*`。

## 自测题

1. **生产者写数据后写 flag 的场景，按决策树应该选什么屏障？**

<details>
<summary>答案</summary>

按决策树：
1. CPU 间同步？ → 是
2. 只约束 Store？ → 是（data 和 flag 都是 Store）
3. → `smp_wmb()`（`dmb ishst`）

但更优选择：能用 LDAR/STLR？ → 是 → 用 `STLR`（Store-Release）写 flag，不需要额外 DMB。C++ 中用 `flag.store(1, std::memory_order_release)`。
</details>

2. **DMA 发送数据的场景，按决策树应该选什么屏障？**

<details>
<summary>答案</summary>

按决策树：
1. CPU ↔ DMA？ → 是
2. 全部？ → 是（需要确保数据完全写入内存）
3. → `mb()`（`dsb sy`）

不能用 DMB（不够强，CPU 不停）。不能用 `smp_mb()`（`dmb ish`，不含 DMA 可见性）。必须用 `mb()`（`dsb sy`）。
</details>

3. **修改 SCTLR 系统寄存器后，按决策树应该选什么屏障？**

<details>
<summary>答案</summary>

按决策树：
1. 系统寄存器/TLB？ → 是
2. → `dsb` + `isb`

修改系统寄存器后必须 DSB（等写入完成）+ ISB（冲刷流水线，确保后续指令在新状态下执行）。例如开 MMU 后 `msr SCTLR_EL1, x0; isb` 中的 ISB 就是这个用途。
</details>

## 参考与延伸

- [§19.1 自旋锁](01-spinlock.md) — CPU 间全屏障案例
- [§19.3 DMA 操作](03-dma.md) — DMA 场屏障案例
- [Ch18 §18.5 Linux 内核屏障 API](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md) — API 对照表
