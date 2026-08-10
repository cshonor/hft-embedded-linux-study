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

| 原则 | 说明 | 举例 |
|------|------|------|
| 选最弱的足够屏障 | ishst 比 ish 轻，ish 比 sy 轻 | Store-Store 用 ishst |
| DMB vs DSB | CPU 间用 DMB，DMA 用 DSB | CPU 间 = dmb, DMA = dsb |
| 作用域 | CPU 间用 ish，DMA 用 osh/sy | 多核 = ish, DMA = sy |
| LDAR/STLR 优先 | 比 DMB 高效，CPU 能更精确优化 | acquire/release |
| 编译器屏障 | 硬件屏障不阻止编译器重排 | barrier() |

### 场景到屏障速查

| 场景 | 屏障 | API | ARM 指令 | 延迟 |
|------|------|-----|---------|------|
| 生产者写 flag | Store-Release | `smp_store_release()` | STLR | ~5ns |
| 消费者读 flag | Load-Acquire | `smp_load_acquire()` | LDAR | ~5ns |
| 生产者写（传统） | Store-Store | `smp_wmb()` | dmb ishst | ~3-5ns |
| 消费者读（传统） | Load-Load | `smp_rmb()` | dmb ishld | ~3-5ns |
| 自旋锁获取后 | 全屏障 | `smp_mb()` | dmb ish | ~5-10ns |
| DMA 启动前 | 完全停住 | `mb()` | dsb sy | ~50-100ns |
| TLB 刷新后 | 停住+取指 | — | dsb ish + isb | ~40-70ns |
| 修改系统寄存器 | 流水线冲刷 | — | isb | ~10-20ns |
| 编译器重排 | 编译器屏障 | `barrier()` | asm volatile("") | ~0ns |

### 最优选择流程

```
Step 1: 能用 LDAR/STLR？
  → 是 → 用 C++ atomic acquire/release → STLR/LDAR（最优）
  → 否 → Step 2

Step 2: 编译器重排还是 CPU 乱序？
  → 编译器 → barrier()（无硬件开销）
  → CPU → Step 3

Step 3: CPU 间还是 CPU↔DMA？
  → CPU 间 → Step 4
  → DMA → Step 5

Step 4: 只需 Store 还是 Load 还是全部？
  → Store → smp_wmb()（dmb ishst）
  → Load → smp_rmb()（dmb ishld）
  → 全部 → smp_mb()（dmb ish）

Step 5: DMA 需要完全停住
  → 写 → wmb()（dsb st）
  → 读 → rmb()（dsb ld）
  → 全部 → mb()（dsb sy）
```

### 过度屏障 vs 不足屏障

| 问题 | 后果 | 示例 |
|------|------|------|
| 过度 | 性能下降 | 消息传递用 `dmb sy`（只需 `ishst`） |
| 不足 | 正确性问题 | DMA 用 `dmb ish`（需 `dsb sy`） |
| 最优 | 正确+性能 | 消息传递用 `STLR/LDAR` |

## HFT 关联

这个决策树是 HFT 无锁编程的实用工具。HFT 开发者在写无锁代码时，按决策树选择屏障可以避免两个极端：1) 过弱屏障（正确性问题，随机失败）；2) 过强屏障（性能问题，不必要停顿）。

### HFT 常用选择

| HFT 场景 | 决策 | 屏障 |
|---------|------|------|
| SPSC push（写） | LDAR/STLR 优先 | STLR（~5ns） |
| SPSC pop（读） | LDAR/STLR 优先 | LDAR（~5ns） |
| 网卡 DMA 发送 | CPU↔DMA | dsb sy（~50-100ns） |
| 页表修改 | TLB 维护 | dsb ish + isb |
| 修改 SCTLR | 系统寄存器 | isb |
| 每核计数器 | 无需屏障 | — |

HFT 最常用的选择是"能用 LDAR/STLR 就用"——C++ `memory_order_acquire`/`release` 自动编译为 LDAR/STLR，是最优选择。DMA 场景（网卡收发）用 `dsb sy`，CPU 间同步用 `dmb ish*`。

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

4. **决策树中"能用 LDAR/STLR 就优先用"的原则，为什么比 DMB 更优？**

<details>
<summary>答案</summary>

LDAR/STLR 比显式 DMB 更优的原因：
1. **单指令**：STLR 替代 `dmb ishst + STR` 两步，LDAR 替代 `LDR + dmb ishld` 两步
2. **CPU 更精确优化**：CPU 知道 acquire/release 意图，只约束必要操作，不需要全屏障停顿
3. **延迟更低**：STLR ~5ns vs `dmb + STR` ~8ns，LDAR ~5ns vs `LDR + dmb` ~8ns
4. **代码更简洁**：C++ atomic 自动编译为 LDAR/STLR，不需要手写汇编
</details>

## 参考与延伸

- [§19.1 自旋锁](01-spinlock.md) — CPU 间全屏障案例
- [§19.3 DMA 操作](03-dma.md) — DMA 场屏障案例
- [Ch18 §18.5 Linux 内核屏障 API](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md) — API 对照表
