# §18.4 Acquire / Release 语义

> **来源：** [Ch18 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

C++11 的 acquire/release 内存序对应 ARM 的 DMB 屏障。ARMv8 的 LDAR/STLR 指令自带 acquire/release 语义，比显式 DMB 更高效——CPU 知道语义意图可以更精确优化。

## 核心要点

### C++ 内存序 → ARM 映射

| C++ 内存序 | ARM 实现 | 含义 |
|-----------|----------|------|
| `memory_order_relaxed` | 无屏障 | 只保证原子性，不保证顺序 |
| `memory_order_acquire` | Load + `dmb ishld` | 后续读不能重排到此 Load 前 |
| `memory_order_release` | `dmb ishst` + Store | 前面写不能重排到此 Store 后 |
| `memory_order_seq_cst` | `dmb ish` + ... | 全序（最强） |

### LDAR / STLR（自带屏障）

| 指令 | 语义 |
|------|------|
| `LDAR` | Load-Acquire：后续访存不能重排到此 Load 前 |
| `STLR` | Store-Release：前面访存不能重排到此 Store 后 |

```c
// 等价于 acquire load
// C++: std::atomic<int> flag;
// int v = flag.load(std::memory_order_acquire);
// ↓ 编译为
ldar w0, [flag_addr]
```

> **LDAR/STLR 比显式 DMB 更高效**：CPU 知道语义意图，可以更精确地优化。

### Acquire/Release 配对

```
生产者:
  store data           // 普通写
  stlr flag = 1        // Store-Release：data 写在 flag 之前可见

消费者:
  ldar v = load flag   // Load-Acquire：读 flag 后再读 data
  load data            // 一定能看到生产者写的值
```

## HFT 关联

LDAR/STLR 是 HFT 无锁编程的首选——比显式 DMB 快 2-5ns（CPU 可以更精确地优化，不需要全屏障停顿）。SPSC 无锁队列用 `store(release)` + `load(acquire)` 配对，编译为 STLR + LDAR，在 A76 上延迟约 5-10ns。C++11 `std::atomic` 的 `memory_order_acquire`/`memory_order_release` 在 ARM 上自动编译为 LDAR/STLR，不需要手写汇编。HFT 代码应优先用 C++ atomic 而非裸汇编屏障，可读性和可维护性更好。

## 自测题

1. **LDAR 和普通 LDR 有什么区别？STLR 和普通 STR 有什么区别？**

<details>
<summary>答案</summary>

- **LDAR**（Load-Acquire）：后续访存**不能重排到此 Load 前**。普通 LDR 无顺序保证。
- **STLR**（Store-Release）：前面访存**不能重排到此 Store 后**。普通 STR 无顺序保证。

LDAR/STLR 自带 acquire/release 语义，不需要额外加 DMB。CPU 知道意图可以更精确优化。
</details>

2. **为什么 LDAR/STLR 比显式 DMB 更高效？**

<details>
<summary>答案</summary>

DMB 是**全屏障**——CPU 需要停住所有相关访存操作，保守地保证顺序。LDAR/STLR 是**单点屏障**——CPU 知道只需要保证这一个 Load/Store 的 acquire/release 语义，可以更精确地只约束相关操作，不需要全停。在 A76 上 LDAR/STLR 比 `LDR + DMB` 快约 2-5ns。
</details>

3. **C++ 的 `memory_order_acquire` 在 ARM 上编译成什么指令？**

<details>
<summary>答案</summary>

`atomic.load(memory_order_acquire)` 编译为 **LDAR**（Load-Acquire）。
`atomic.store(memory_order_release)` 编译为 **STLR**（Store-Release）。

不需要额外的 DMB 指令——LDAR/STLR 自带屏障语义。这也是为什么 C++ atomic 在 ARM 上性能好——编译器选择最优的指令实现。
</details>

## 参考与延伸

- [§18.2 三条屏障指令](02-three-barriers.md) — DMB/DSB/ISB 详解
- [§18.3 典型场景](03-typical-scenarios.md) — 消息传递场景的 acquire/release
- [Ch19 §19.6 HFT 中的屏障使用](../../chapter-19-barrier-usage/notes/section-0-本章完整概述.md) — SPSC 队列实战
