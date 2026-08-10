# §19.7 易错点清单

> **来源：** [Ch19 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

屏障使用的 5 个常见错误：屏障加错位置、屏障类型选错、忘加编译器屏障、过度使用 dmb sy、不读内核源码就写裸屏障。

## 核心要点

| # | 易错点 | 后果 | 修复 |
|---|--------|------|------|
| 1 | 屏障加错位置 | 加在临界区外 vs 内，效果完全不同 | 仔细分析屏障应保护哪些访存 |
| 2 | 屏障类型选错 | DMA 用了 smp_mb（不够强，不含 DMA 可见性） | DMA 用 mb()（dsb sy） |
| 3 | 忘加编译器屏障 | 硬件屏障不阻止编译器重排 | barrier() 或 volatile |
| 4 | 过度使用 dmb sy | 很多场景只需 ishst/ishld，全屏障性能损失大 | 选最弱的足够屏障 |
| 5 | 不读内核源码就写裸屏障 | Linux 有完善的屏障 API，优先用 | 用 smp_mb()/mb() 等 API |

### 位置对比例子

```c
// 错误：屏障在临界区外
lock();
shared_var = 42;
unlock();
smp_mb();  // ← 错！屏障应该在 lock 后和 unlock 前

// 正确：屏障在临界区内
lock();
smp_mb();  // ← 获取锁后
shared_var = 42;
smp_mb();  // ← 释放锁前
unlock();
```

## HFT 关联

屏障位置错误是 HFT 无锁编程中第二常见的 bug（第一是忘加屏障）。在 SPSC 队列中，`release` 屏障必须加在 `buffer[w % N] = val` **之后**、`write_idx.store` **之前**——如果加反了（先 store index 再写 buffer），消费者看到新 index 但读到旧 buffer 值。HFT 建议用 C++ `std::atomic` 的内存序参数代替裸 DMB——编译器自动在正确位置生成 STLR/LDAR，避免手动位置错误。

## 自测题

1. **自旋锁的屏障应该加在 lock/unlock 的内侧还是外侧？**

<details>
<summary>答案</summary>

**内侧**。
- `smp_mb()` 在 `lock()` **之后**（内侧）：保证临界区访存不重排到锁获取之前
- `smp_mb()` 在 `unlock()` **之前**（内侧）：保证临界区写在锁释放之前可见

加在外侧（lock 之前 / unlock 之后）无法保护临界区——屏障约束的访存范围不对。
</details>

2. **DMA 场景误用 `smp_mb()`（dmb ish）而非 `mb()`（dsb sy），会怎样？**

<details>
<summary>答案</summary>

`smp_mb()` = `dmb ish` 不含 DMA 可见性——DMB 只保证 CPU 间访存顺序，不保证数据到达 DMA 可观察的点（PoC）。结果：`start_dma()` 可能先执行，DMA 读到不完整数据。必须用 `mb()` = `dsb sy` 完全停住 CPU 等数据写入完成。这是 `smp_*` 和非 `smp_*` 的关键区别。
</details>

3. **为什么"不读内核源码就写裸屏障"是易错点？**

<details>
<summary>答案</summary>

Linux 内核有完善的屏障 API（`smp_mb`/`mb`/`smp_wmb`/`wmb` 等），经过架构专家验证，正确处理了编译器屏障、作用域、DMB/DSB 选择等细节。手写裸 `dmb`/`dsb` 容易遗漏编译器屏障、选错作用域或强度。优先用内核 API，只有在用户态 HFT 中才需要手写（且应优先用 C++ `std::atomic`）。
</details>

## 参考与延伸

- [§19.5 屏障选择决策树](05-decision-tree.md) — 系统化选择方法
- [§19.1 自旋锁](01-spinlock.md) — 屏障位置的正确示例
- [Ch18 §18.7 易错点](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md) — 屏障基础错误
