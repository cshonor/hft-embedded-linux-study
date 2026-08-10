# §18.7 易错点清单

> **来源：** [Ch18 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

内存屏障的 6 个常见错误：忘加屏障、DMB/DSB 混用、作用域选错、忘加编译器屏障、过度使用屏障、屏障加错位置。

## 核心要点

| # | 易错点 | 后果 | 修复 |
|---|--------|------|------|
| 1 | 忘加屏障 | 乱序导致数据不一致（最难 debug 的 bug） | 分析所有多核共享数据的访问顺序 |
| 2 | DMB 和 DSB 混用 | DMB 不停 CPU，DMA 场景可能不够强 | DMA 用 DSB，CPU 间用 DMB |
| 3 | 作用域选错 | `nsh` 只管本核，多核场景要用 `ish` | 多核用 `ish`，DMA 用 `osh`/`sy` |
| 4 | 忘加编译器屏障 | 硬件屏障不阻止编译器重排 | `barrier()` 或 `volatile` |
| 5 | 过度使用屏障 | 性能下降。能用 acquire/release 就不用 seq_cst | 选最弱的足够屏障 |
| 6 | 屏障加错位置 | 加在临界区外 vs 内，效果完全不同 | 仔细分析屏障应保护哪些访存 |

### 常见代码错误对比

| # | 错误代码 | 正确代码 | 问题 |
|---|---------|---------|------|
| 1 | DMA: `wmb()` 后 `start_dma()` | DMA: `mb()`(=dsb sy) 后 `start_dma()` | wmb 只 dsb st，不够强 |
| 2 | 多核: `dmb nsh` | 多核: `dmb ish` | nsh 只管本核 |
| 3 | `dmb ish` 后用 `volatile` 但忘 `barrier()` | `smp_mb()` 内含 `barrier()` | DMB 不阻止编译器 |
| 4 | 消息传递: `dmb sy` | 消息传递: `dmb ishst` | sy 过度，ishst 足够 |
| 5 | 屏障在 `unlock()` 之后 | 屏障在 `unlock()` 之前 | 位置错误 |

### 屏障强度选择速查

| 场景 | 正确选择 | 常见错误 | 错误后果 |
|------|---------|---------|---------|
| CPU 间 Store-Store | `dmb ishst` | `dmb sy` | 过度，性能损失 |
| CPU 间全屏障 | `dmb ish` | `dsb sy` | 过度，完全停住 |
| CPU↔DMA | `dsb sy` | `dmb ish` | 不够强，DMA 读到旧数据 |
| 系统寄存器后 | `isb` | 只用 `dsb` | 流水线未冲刷 |
| TLB 刷新后 | `dsb ish` + `isb` | 只用 `dsb` | 流水线中有旧翻译 |

### 调试技巧

| 症状 | 可能原因 | 检查方向 |
|------|----------|---------|
| 多核数据"偶尔"不一致 | 忘加屏障 | 检查共享变量访问是否有屏障 |
| DMA 数据偶尔错误 | DMB 不够强，应用 DSB | 检查 DMA 场景是否用 `mb()` |
| 屏障加了但没效果 | 编译器把代码重排了 | 加 `barrier()` 或 `volatile` |
| 性能异常低 | 过度使用 `dmb sy` | 改用 `ishst`/`ishld` |
| 临界区数据被意外修改 | 屏障位置错误 | 屏障应在 lock 后/unlock 前 |
| x86 正确 ARM 失败 | x86 TSO 比 ARM 强 | 加 ARM 显式屏障 |

## HFT 关联

忘加屏障是 HFT 无锁编程中最隐蔽的 bug——代码在 x86 上完美运行（TSO 较强），在 ARM 上随机失败（弱序模型）。这种 bug 极难复现和调试，因为乱序是概率性的。

### HFT 屏障审查清单

```c
// ✓ 检查项 1：跨核共享数据访问是否有屏障
data = 42;
smp_wmb();         // ✓ Store-Store 屏障
flag = 1;

// ✓ 检查项 2：DMA 场景是否用 DSB（不是 DMB）
prepare_buffer();
mb();              // ✓ DSB sy（不是 smp_mb！）
start_dma();

// ✓ 检查项 3：作用域是否正确（多核用 ish）
dmb ish;           // ✓ Inner Shareable
dmb nsh;           // ✗ 只管本核！

// ✓ 检查项 4：是否加了编译器屏障
asm volatile("" ::: "memory");  // ✓ 或用 smp_mb()

// ✓ 检查项 5：是否选了最弱的足够屏障
dmb ishst;         // ✓ 只需 Store-Store
dmb sy;            // ✗ 过度

// ✓ 检查项 6：屏障位置是否正确
lock();
smp_mb();          // ✓ lock 后
// ...临界区...
smp_mb();          // ✓ unlock 前
unlock();
```

HFT 开发建议：1) 用 C++ `std::atomic` 替代裸屏障（编译器自动选择最优指令）；2) 代码审查时检查所有跨核共享数据的访问顺序；3) 在 ARM 硬件上做压力测试（QEMU 可能不暴露弱序问题）。过度使用 `dmb sy` 在 HFT 中是常见性能问题——很多场景只需 `ishst`/`ishld`。

## 自测题

1. **代码在 x86 上正确但在 ARM 上偶尔失败，最可能的原因是什么？**

<details>
<summary>答案</summary>

**忘加内存屏障**。x86 的 TSO 内存模型较强（Store-Store 不重排），代码"碰巧正确"。ARM 是弱序模型（Store-Store 可重排），不加屏障会导致乱序，消费者可能看到 flag 更新但 data 未更新。这种 bug 极难复现——乱序是概率性的，取决于 CPU 负载、cache 状态等。
</details>

2. **加了 `dmb ish` 但行为仍不正确，可能的原因是什么？**

<details>
<summary>答案</summary>

可能原因：
1. **编译器重排**：DMB 只约束 CPU 乱序，不阻止编译器重排。需要加 `barrier()` 或用 `volatile`
2. **作用域不够**：`ish` 只在 Inner Shareable 范围有效，如果涉及 DMA 需要用 `osh`/`sy`
3. **DMB 不够强**：如果场景需要"完成"而非"顺序"（如 DMA），应该用 DSB
4. **屏障位置错误**：加在了错误的位置（如临界区外）
</details>

3. **如何避免过度使用屏障？给出 3 条建议。**

<details>
<summary>答案</summary>

1. **选最弱的足够屏障**：Store-Store 用 `ishst`，Load-Load 用 `ishld`，不用 `sy`（全屏障）
2. **用 LDAR/STLR 替代 DMB**：acquire/release 语义比显式 DMB 更高效
3. **用 C++ atomic**：`memory_order_acquire`/`release` 比 `seq_cst` 更轻，编译器自动选最优指令
4. **减少共享**：每核独立数据不需要屏障（percpu 变量）
</details>

4. **DMA 场景误用 `smp_mb()`（dmb ish）而非 `mb()`（dsb sy），会怎样？**

<details>
<summary>答案</summary>

`smp_mb()` = `dmb ish` 不含 DMA 可见性——DMB 只保证 CPU 间访存顺序，不保证数据到达 DMA 可观察的点（PoC）。结果：`start_dma()` 可能先执行，DMA 读到不完整数据。必须用 `mb()` = `dsb sy` 完全停住 CPU 等数据写入完成。这是 `smp_*` 和非 `smp_*` 的关键区别。
</details>

## 参考与延伸

- [§18.2 三条屏障指令](02-three-barriers.md) — DMB/DSB/ISB 选择
- [§18.4 Acquire/Release](04-acquire-release.md) — 更高效的替代方案
- [Ch19 §19.5 屏障选择决策树](../../chapter-19-barrier-usage/notes/section-0-本章完整概述.md) — 系统化的屏障选择方法
