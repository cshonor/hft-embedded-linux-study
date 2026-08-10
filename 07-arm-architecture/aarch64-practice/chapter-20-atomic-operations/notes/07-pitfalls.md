# §20.7 易错点清单

> **来源：** [Ch20 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

原子操作的 6 个常见错误：LDXR 后不做 STXR、CAS 循环太长（livelock）、WFE 被误唤醒、忘加屏障、误以为 atomic_read 保证可见性、原子变量伪共享。

## 核心要点

| # | 易错点 | 后果 | 修复 |
|---|--------|------|------|
| 1 | LDXR 后不做 STXR | 监视器残留 → 后续 LDXR 异常 | LDXR 后必须 STXR 或 CLREX |
| 2 | CAS 循环太长 | 高竞争时活锁（livelock） | 退避策略或用 LSE |
| 3 | WFE 被误唤醒 | 醒来后假设锁已释放 → 错误 | 重新检查条件 |
| 4 | 忘加屏障 | 原子操作不保证和其他访存的顺序 | 用 LDAXR/STLXR 或 smp_mb |
| 5 | 用 atomic_read 就以为安全 | atomic_read 只保证编译器不优化 | 用 smp_load_acquire 或 DMB |
| 6 | 原子变量伪共享 | 同一 cache line 的原子操作互相干扰 | aligned(64) 独占 cache line |

### 常见代码错误对比

| # | 错误代码 | 正确代码 | 问题 |
|---|---------|---------|------|
| 1 | LDXR 后不匹配直接返回 | LDXR 后不匹配 → CLREX → 返回 | 监视器残留 |
| 2 | CAS 无退避循环 | CAS 失败后 wfe/cpu_relax 再重试 | livelock |
| 3 | WFE 后假设锁已释放 | WFE 后 LDXR 重新检查 | 误唤醒 |
| 4 | atomic_add 后直接读其他变量 | atomic_add + smp_mb__after_atomic | 缺屏障 |
| 5 | `atomic_read(&v)` 假设读到最新值 | `smp_load_acquire(&v)` | 缺可见性保证 |
| 6 | 两个 atomic 在同一 struct | `alignas(64)` 每个独占 cache line | 伪共享 |

### 调试技巧

| 症状 | 可能原因 | 检查方向 |
|------|----------|---------|
| LDXR 行为异常 | 上次 LDXR 没配 STXR/CLREX | 检查 CAS 不匹配路径是否有 CLREX |
| CAS 性能极差 | 高竞争 livelock | 改用 LSE 或退避 |
| WFE 醒来但锁没释放 | 被中断等误唤醒 | 重新检查条件 |
| 原子操作后数据不一致 | 忘加屏障 | 用 LDAXR/STLXR 或 smp_mb__after_atomic |
| 读到旧值 | atomic_read 不保证可见性 | 用 smp_load_acquire |
| STXR 反复失败 | 伪共享 | aligned(64) |

### CAS livelock 场景

```
高竞争 livelock：
T0: CPU0 LDXR [addr] → 标记独占
T1: CPU1 LDXR [addr] → 标记独占
T2: CPU0 STXR → 成功（CPU1 监视器被清除）
T3: CPU1 STXR → 失败 → 重试
T4: CPU1 LDXR → 标记独占（清除 CPU0 的监视器）
T5: CPU0 下一轮 LDXR → 标记独占
T6: CPU0 STXR → 可能失败（CPU1 刚 LDXR）
→ 循环往复，可能 livelock
```

### 解决 livelock 的方法

| 方法 | 延迟 | 说明 |
|------|------|------|
| 退避（WFE/cpu_relax） | 中等 | 失败后等一会儿再重试 |
| LSE 单指令 | 低 | 无循环，硬件保证完成 |
| SPSC 模式 | 最低 | 无竞争，每核独立 |
| 分片（striping） | 低 | 将竞争分散到多个 slot |

## HFT 关联

这些错误在 HFT 无锁编程中都有致命后果。CAS livelock 是 HFT 最怕的——高竞争时 CAS 反复失败，延迟不可预测（可能微秒级），而 HFT 需要确定性延迟。

### HFT 无锁编程安全清单

```c
// ✓ 检查项 1：LDXR 后是否配 STXR 或 CLREX
// CAS 不匹配路径必须有 CLREX
1:  ldxr w1, [x0]
    cmp  w1, w2
    b.ne 2f
    stxr w3, w4, [x0]
    cbnz w3, 1b
    b 3f
2:  clrex              // ← 不匹配时清除监视器
3:  ...

// ✓ 检查项 2：CAS 循环是否有退避
do {
    expected = atomic_load(v);
    // ... 计算 new_val ...
    if (atomic_cas(v, &expected, new_val)) break;
    cpu_relax();       // ← 退避
} while (1);

// ✓ 检查项 3：原子变量是否独占 cache line
struct alignas(64) { std::atomic<int> counter; } per_cpu[N];

// ✓ 检查项 4：原子操作后是否需要屏障
atomic_add(1, &counter);
smp_mb__after_atomic();  // 或用 atomic_add_return

// ✓ 检查项 5：是否用 LSE 避免 livelock
// 编译时 -march=armv8.1-a 启用 LSE
```

解决方案：1) 用 SPSC 模式避免竞争；2) 用 LSE 单指令（无 livelock）；3) 加退避（`wfe` 或 `cpu_relax`）。忘加屏障是最隐蔽的 bug——原子操作本身是原子的，但与其他访存的顺序不保证。HFT 代码应优先用 C++ `std::atomic` 的内存序参数明确语义。

## 自测题

1. **LDXR 后不执行 STXR 也不执行 CLREX，会有什么问题？**

<details>
<summary>答案</summary>

独占监视器**残留**——LDXR 标记了独占但不释放。后续的 LDXR 可能行为异常（取决于架构实现，可能标记失败或影响其他核的监视器）。在某些 ARM 实现中，监视器会自动过期（如发生上下文切换时自动 CLREX），但最佳实践是显式 CLREX。Ch7 的案例就涉及 LDXR 后不做 STXR 导致的问题。
</details>

2. **CAS 在高竞争时为什么性能暴跌？如何修复？**

<details>
<summary>答案</summary>

高竞争时多个核同时 LDXR 同一地址，只有一个 STXR 成功，其他失败重试——竞争越激烈重试越多，甚至 livelock（反复失败）。修复：
1. **用 LSE**（如 CAS 单指令，无循环重试）
2. **退避策略**：失败后 `wfe` 或 `cpu_relax()` 等待一会儿再重试
3. **避免竞争**：改用 SPSC 模式（每核独立队列，无 CAS 竞争）
4. **减少临界区**：CAS 操作越快，竞争窗口越小
</details>

3. **`atomic_read(&v)` 能保证读到其他核刚写的值吗？为什么？**

<details>
<summary>答案</summary>

**不能**。`atomic_read` 展开为 `READ_ONCE`（`volatile` 读），只阻止编译器优化（不缓存到寄存器），不保证多核可见性。其他核写了 `v` 后，写操作可能在 cache 中还没传播到当前核（ARM 弱序模型，Store-Load 可重排）。要保证读到最新值，用 `smp_load_acquire(&v)`（编译为 LDAR，带 acquire 语义）或 `smp_rmb()` + `READ_ONCE`。
</details>

4. **两个核分别对两个不相关原子变量做 fetch_add，为什么延迟很高？**

<details>
<summary>答案</summary>

**伪共享**。两个原子变量在同一 cache line 中，LDXR/STXR 以 cache line 为粒度监视。一个核的 LDXR 标记整个 cache line 独占，另一个核的 LDXR 清除了前者的监视器 → STXR 反复失败 → 重试 → 延迟暴增。修复：用 `alignas(64)` 让每个原子变量独占一个 cache line。
</details>

## 参考与延伸

- [§20.1 独占监视器](01-exclusive-monitor.md) — LDXR/STXR 原理
- [§20.5 Linux 原子操作 API](05-linux-atomic-api.md) — atomic_read 的局限
- [Ch18 §18.7 易错点](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md) — 屏障相关错误
