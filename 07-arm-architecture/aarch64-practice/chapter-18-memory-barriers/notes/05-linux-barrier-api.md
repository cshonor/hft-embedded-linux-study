# §18.5 Linux 内核屏障 API

> **来源：** [Ch18 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

Linux 内核封装的屏障 API：smp_mb/smp_rmb/smp_wmb（CPU 间）和 mb/rmb/wmb（含 DMA），以及 barrier()（编译器屏障）。优先用内核 API 而非裸汇编屏障。

## 核心要点

### Linux 屏障 API 对照

| API | 展开为 | 用途 |
|-----|--------|------|
| `smp_mb()` | `dmb ish` | 全屏障（SMP） |
| `smp_rmb()` | `dmb ishld` | 读屏障 |
| `smp_wmb()` | `dmb ishst` | 写屏障 |
| `mb()` | `dsb sy` | 全屏障（含 DMA） |
| `rmb()` | `dsb ld` | 读屏障（含 DMA） |
| `wmb()` | `dsb st` | 写屏障（含 DMA） |
| `barrier()` | 编译器屏障 | 只阻止编译器重排，不加硬件屏障 |

### smp_* vs 非 smp_*

| 类型 | 展开为 | 场景 |
|------|--------|------|
| `smp_mb()` | `dmb ish` | **CPU 间**同步（DMB 足够） |
| `mb()` | `dsb sy` | **CPU ↔ DMA**同步（需 DSB） |

> `smp_*` 用于 CPU 间同步；不带 `smp_` 的用于 CPU 与 DMA 同步。

### 编译器屏障

```c
// barrier() 阻止编译器重排，但不加硬件屏障
barrier();
// 等价于 asm volatile("" ::: "memory")

// volatile 也阻止编译器优化，但不阻止 CPU 乱序
volatile int flag;
```

> **硬件屏障不阻止编译器重排**！需要 `barrier()` 或 `volatile` 配合。

## HFT 关联

Linux 屏障 API 在用户态 HFT 中不直接可用（内核 API），但理解其映射关系有助于在用户态选择正确的 C++ atomic 内存序。`smp_wmb()` = `dmb ishst` ≈ C++ `memory_order_release`；`smp_rmb()` = `dmb ishld` ≈ `memory_order_acquire`。`barrier()`（编译器屏障）在用户态用 `asm volatile("" ::: "memory")` 实现，防止编译器把关键变量优化到寄存器中。HFT 代码中 `volatile` + `dmb` 是经典模式，但 `std::atomic` 更安全。

## 自测题

1. **`smp_mb()` 和 `mb()` 的区别？分别展开为什么指令？**

<details>
<summary>答案</summary>

- `smp_mb()` = `dmb ish`：CPU 间全屏障，DMB 足够（CPU 间只需顺序保证，不需停住）
- `mb()` = `dsb sy`：含 DMA 的全屏障，需要 DSB（DMA 需要完全等访存完成）

`smp_*` 用于 CPU 间同步（DMB），不带 `smp_` 用于 CPU↔DMA 同步（DSB）。
</details>

2. **`barrier()` 的作用是什么？为什么硬件屏障不够？**

<details>
<summary>答案</summary>

`barrier()` 是**编译器屏障**，阻止编译器把访存操作重排或优化到寄存器中。硬件屏障（DMB/DSB）只约束 CPU 的乱序执行，**不阻止编译器重排**。如果只有硬件屏障，编译器可能在编译时把 Store 重排到屏障前，硬件屏障就失效了。因此需要 `barrier()` 配合硬件屏障使用，或用 `volatile` 防止编译器优化。
</details>

3. **`smp_wmb()` 展开为什么？为什么用 ishst 而不是 sy？**

<details>
<summary>答案</summary>

`smp_wmb()` = `dmb ishst`。
- **ish**（Inner Shareable）：CPU 间同步只需 Inner Shareable，不需要全系统（sy 更重）
- **st**（仅 Store）：写屏障只需约束 Store-Store，不需要约束 Load

`ishst` 是最轻量的 Store-Store 屏障，在保证正确性的前提下最小化性能损失。
</details>

## 参考与延伸

- [§18.2 三条屏障指令](02-three-barriers.md) — DMB/DSB 详解
- [§18.4 Acquire/Release](04-acquire-release.md) — LDAR/STLR 替代显式屏障
- [Ch19 §19.1 自旋锁](../../chapter-19-barrier-usage/notes/section-0-本章完整概述.md) — smp_mb 在自旋锁中的使用
