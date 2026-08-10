# §19.1 案例一：自旋锁获取/释放

> **来源：** [Ch19 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

Linux 自旋锁的屏障使用：获取锁后 smp_mb()（保证临界区不重排到锁前），释放锁前 smp_mb()（保证临界区写在释放前可见）。现代 ARM 用 LDAR/STLR 替代显式 DMB。

## 核心要点

### 自旋锁屏障模式

```c
// Linux spinlock 简化版
void spin_lock(spinlock_t *lock) {
    while (atomic_cmpxchg(&lock->val, 0, 1) != 0)
        ;  // 自旋等待
    smp_mb();  // ← 获取锁后加屏障
}

void spin_unlock(spinlock_t *lock) {
    smp_mb();  // ← 释放锁前加屏障
    atomic_set(&lock->val, 0);
}
```

### 为什么需要屏障？

| 位置 | 屏障 | 原因 |
|------|------|------|
| lock 后 | `smp_mb()` | 保证临界区内的访存不被重排到锁获取之前（其他核可能还在临界区） |
| unlock 前 | `smp_mb()` | 保证临界区内的写在锁释放前对其他核可见 |

### 现代 ARM 优化

> **现代 ARM 内核**：用 LDAR/STLR 替代显式 DMB，更高效。

```asm
// 传统：LDXR + STXR + DMB
ldxr w1, [x0]        ; 独占读
cmp w1, #0
b.ne retry
stxr w2, w1, [x0]    ; 独占写
cbnz w2, retry
dmb ish              ; ← 显式屏障

// 现代：LDAXR + STLXR（自带 acquire/release）
ldaxr w1, [x0]       ; Load-Acquire 独占读
cmp w1, #0
b.ne retry
stlxr w2, w1, [x0]   ; Store-Release 独占写
cbnz w2, retry
// 无需额外 DMB！
```

## HFT 关联

自旋锁在 HFT 中应尽量避免——忙等浪费 CPU 周期，且锁竞争会导致延迟抖动。但如果用自旋锁保护极短临界区（如更新一个计数器），理解屏障位置很重要。获取锁后的 `smp_mb()` 确保临界区代码不会在编译器或 CPU 层面被重排到锁获取之前——否则其他核可能看到临界区的部分修改。HFT 中更好的替代方案是 RCU（读多写少场景）或 percpu 变量（每核独立计数）。

## 自测题

1. **自旋锁获取后为什么要加 smp_mb()？不加会怎样？**

<details>
<summary>答案</summary>

获取锁后加 `smp_mb()` 保证**临界区内的访存不被重排到锁获取之前**。如果不加：CPU/编译器可能把临界区内的 Store 重排到 `cmpxchg` 之前 → 其他核在持有锁时看到部分修改 → 数据不一致。例如 `shared_var = 42` 可能被重排到 `lock()` 之前执行，其他核在临界区内看到 `shared_var` 被意外修改。
</details>

2. **释放锁前为什么要加 smp_mb()？不加会怎样？**

<details>
<summary>答案</summary>

释放锁前加 `smp_mb()` 保证**临界区内的写在锁释放前对其他核可见**。如果不加：CPU 可能把 `lock = 0`（释放）重排到 `shared_var = 42`（临界区写）之前 → 其他核看到锁已释放但数据还没写完 → 拿到锁后读到旧值。
</details>

3. **现代 ARM 用 LDAXR/STLXR 替代 LDXR/STXR + DMB，有什么好处？**

<details>
<summary>答案</summary>

LDAXR（Load-Acquire Exclusive）/ STLXR（Store-Release Exclusive）**自带 acquire/release 语义**，不需要额外 DMB 指令。好处：
1. **减少指令数**：不需要单独的 DMB
2. **CPU 更精确优化**：知道 acquire/release 意图，只约束必要操作
3. **延迟更低**：约快 2-5ns（少一条 DMB 指令 + 更精确的约束）
</details>

## 参考与延伸

- [§19.5 屏障选择决策树](05-decision-tree.md) — 自旋锁在决策树中的位置
- [Ch18 §18.2 三条屏障指令](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md) — smp_mb 的展开
- [Ch20 §20.4 WFE/SEV](../../chapter-20-atomic-operations/notes/section-0-本章完整概述.md) — WFE 低功耗自旋锁
