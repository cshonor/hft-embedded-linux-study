# §18.5 Linux 内核屏障 API

> **来源：** [Ch18 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

Linux 内核封装的屏障 API：smp_mb/smp_rmb/smp_wmb（CPU 间）和 mb/rmb/wmb（含 DMA），以及 barrier()（编译器屏障）。本节给出 API 到 ARM 指令的完整映射、smp_* vs 非 smp_* 的区别，以及编译器屏障的重要性。

## 核心要点

### Linux 屏障 API 完整对照

| API | 展开为 | 约束 | 作用域 | 场景 |
|-----|--------|------|--------|------|
| `smp_mb()` | `dmb ish` | Load+Store | Inner Shareable | CPU 间全屏障 |
| `smp_rmb()` | `dmb ishld` | 仅 Load | Inner Shareable | CPU 间读屏障 |
| `smp_wmb()` | `dmb ishst` | 仅 Store | Inner Shareable | CPU 间写屏障 |
| `mb()` | `dsb sy` | 完全停住 | Full system | CPU↔DMA 全屏障 |
| `rmb()` | `dsb ld` | 完全停住(读) | Full system | CPU↔DMA 读屏障 |
| `wmb()` | `dsb st` | 完全停住(写) | Full system | CPU↔DMA 写屏障 |
| `barrier()` | 编译器屏障 | — | 编译时 | 阻止编译器重排 |
| `smp_mb__after_atomic()` | `dmb ish` | Load+Store | Inner Shareable | 原子操作后补屏障 |

### smp_* vs 非 smp_*

| 类型 | 展开为 | 场景 | DMB vs DSB |
|------|--------|------|-----------|
| `smp_mb()` | `dmb ish` | **CPU 间**同步 | DMB（足够） |
| `mb()` | `dsb sy` | **CPU ↔ DMA**同步 | DSB（必须停住） |
| `smp_wmb()` | `dmb ishst` | CPU 间写屏障 | DMB（足够） |
| `wmb()` | `dsb st` | CPU↔DMA 写屏障 | DSB（必须停住） |

> `smp_*` 用于 CPU 间同步；不带 `smp_` 的用于 CPU 与 DMA 同步。

### 编译器屏障

```c
// barrier() 阻止编译器重排，但不加硬件屏障
barrier();
// 等价于 asm volatile("" ::: "memory")

// volatile 也阻止编译器优化，但不阻止 CPU 乱序
volatile int flag;

// 完整保护：编译器屏障 + 硬件屏障
flag = 1;       // volatile 写
smp_wmb();      // 硬件写屏障 + 内含 barrier()
```

### 屏障类型对比

| 屏障类型 | 阻止编译器重排？ | 阻止 CPU 乱序？ | 指令 |
|---------|----------------|----------------|------|
| `barrier()` | **是** | 否 | `asm volatile("" ::: "memory")` |
| `volatile` | **是**（部分） | 否 | 变量声明 |
| `smp_mb()` | **是** | **是** | `dmb ish`（含 barrier） |
| `dmb ish` | **否** | **是** | 纯硬件屏障 |

> **硬件屏障不阻止编译器重排**！需要 `barrier()` 或 `volatile` 配合。
> Linux 的 `smp_mb()` 等宏内含 `barrier()`，所以同时阻止编译器和 CPU 重排。

### smp_store_release / smp_load_acquire

```c
// Linux 也提供 acquire/release API（用 LDAR/STLR）
smp_store_release(&flag, 1);    // → STLR
val = smp_load_acquire(&flag);  // → LDAR

// 等价于
smp_wmb();
WRITE_ONCE(flag, 1);
// 或
val = READ_ONCE(flag);
smp_rmb();
```

### API 选择决策

| 需求 | API | 说明 |
|------|-----|------|
| CPU 间 Store-Store 有序 | `smp_wmb()` | 最轻量写屏障 |
| CPU 间 Load-Load 有序 | `smp_rmb()` | 最轻量读屏障 |
| CPU 间全屏障 | `smp_mb()` | 自旋锁等 |
| CPU↔DMA 同步 | `mb()`/`wmb()`/`rmb()` | 用 DSB |
| 阻止编译器重排 | `barrier()` | 无硬件开销 |
| Acquire/Release | `smp_load_acquire()`/`smp_store_release()` | 用 LDAR/STLR |
| 原子操作后补屏障 | `smp_mb__after_atomic()` | relaxed 原子操作 |

## HFT 关联

Linux 屏障 API 在用户态 HFT 中不直接可用（内核 API），但理解其映射关系有助于在用户态选择正确的 C++ atomic 内存序。

### 用户态 HFT 屏障实现

```c
// 用户态编译器屏障
#define barrier() asm volatile("" ::: "memory")

// 用户态 Store-Release（用 STLR）
static inline void smp_store_release(int *p, int v) {
    asm volatile("stlr %w1, %0" : "=Q"(*p) : "r"(v) : "memory");
}

// 用户态 Load-Acquire（用 LDAR）
static inline int smp_load_acquire(const int *p) {
    int v;
    asm volatile("ldar %w0, %1" : "=r"(v) : "Q"(*p) : "memory");
    return v;
}

// 或直接用 C++ std::atomic（推荐）
std::atomic<int> flag;
flag.store(1, std::memory_order_release);   // → STLR
int v = flag.load(std::memory_order_acquire); // → LDAR
```

`smp_wmb()` = `dmb ishst` ≈ C++ `memory_order_release`；`smp_rmb()` = `dmb ishld` ≈ `memory_order_acquire`。`barrier()`（编译器屏障）在用户态用 `asm volatile("" ::: "memory")` 实现，防止编译器把关键变量优化到寄存器中。HFT 代码中 `volatile` + `dmb` 是经典模式，但 `std::atomic` 更安全。

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

4. **`smp_store_release()` 和 `smp_wmb() + WRITE_ONCE()` 有什么区别？**

<details>
<summary>答案</summary>

- `smp_store_release()`：用 `STLR` 单指令（Store-Release），~5ns
- `smp_wmb() + WRITE_ONCE()`：用 `dmb ishst + STR` 两步，~8ns

两者语义等价（都保证前面的访存在 Store 之前可见），但 `smp_store_release()` 用 STLR 更高效（少一条指令 + CPU 更精确优化）。Linux 内核优先用 `smp_store_release()`。
</details>

## 参考与延伸

- [§18.2 三条屏障指令](02-three-barriers.md) — DMB/DSB 详解
- [§18.4 Acquire/Release](04-acquire-release.md) — LDAR/STLR 替代显式屏障
- [Ch19 §19.1 自旋锁](../../chapter-19-barrier-usage/notes/section-0-本章完整概述.md) — smp_mb 在自旋锁中的使用
