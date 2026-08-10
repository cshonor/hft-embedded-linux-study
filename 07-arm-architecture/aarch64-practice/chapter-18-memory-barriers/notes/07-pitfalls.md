# §18.7 易错点清单

> **来源：** [Ch18 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

内存屏障的 5 个常见错误：忘加屏障、DMB/DSB 混用、作用域选错、忘加编译器屏障、过度使用屏障。

## 核心要点

| # | 易错点 | 后果 | 修复 |
|---|--------|------|------|
| 1 | 忘加屏障 | 乱序导致数据不一致（最难 debug 的 bug） | 分析所有多核共享数据的访问顺序 |
| 2 | DMB 和 DSB 混用 | DMB 不停 CPU，DMA 场景可能不够强 | DMA 用 DSB，CPU 间用 DMB |
| 3 | 作用域选错 | `nsh` 只管本核，多核场景要用 `ish` | 多核用 `ish`，DMA 用 `osh`/`sy` |
| 4 | 编译器重排 | 硬件屏障不阻止编译器重排 | `barrier()` 或 `volatile` |
| 5 | 过度使用屏障 | 性能下降。能用 acquire/release 就不用 seq_cst | 选最弱的足够屏障 |

### 调试技巧

| 症状 | 可能原因 |
|------|----------|
| 多核数据"偶尔"不一致 | 忘加屏障（最难复现的 bug） |
| DMA 数据偶尔错误 | DMB 不够强，应该用 DSB |
| 屏障加了但没效果 | 编译器把代码重排了，需 `barrier()` |
| 性能异常低 | 过度使用 `dmb sy`，应改用 `ishst` |

## HFT 关联

忘加屏障是 HFT 无锁编程中最隐蔽的 bug——代码在 x86 上完美运行（TSO 较强），在 ARM 上随机失败（弱序模型）。这种 bug 极难复现和调试，因为乱序是概率性的。HFT 开发建议：1) 用 C++ `std::atomic` 替代裸屏障（编译器自动选择最优指令）；2) 代码审查时检查所有跨核共享数据的访问顺序；3) 在 ARM 硬件上做压力测试（QEMU 可能不暴露弱序问题）。过度使用 `dmb sy` 在 HFT 中是常见性能问题——很多场景只需 `ishst`/`ishld`。

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

<detail>

<summary>答案</summary>

1. **选最弱的足够屏障**：Store-Store 用 `ishst`，Load-Load 用 `ishld`，不用 `sy`（全屏障）
2. **用 LDAR/STLR 替代 DMB**：acquire/release 语义比显式 DMB 更高效
3. **用 C++ atomic**：`memory_order_acquire`/`release` 比 `seq_cst` 更轻，编译器自动选最优指令
4. **减少共享**：每核独立数据不需要屏障（percpu 变量）
</details>

## 参考与延伸

- [§18.2 三条屏障指令](02-three-barriers.md) — DMB/DSB/ISB 选择
- [§18.4 Acquire/Release](04-acquire-release.md) — 更高效的替代方案
- [Ch19 §19.5 屏障选择决策树](../../chapter-19-barrier-usage/notes/section-0-本章完整概述.md) — 系统化的屏障选择方法
