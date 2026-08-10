# §18.6 实验要点

> **来源：** [Ch18 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

本章以案例分析为主，无独立编号实验。关键案例：消息传递、自旋锁、邮箱传递、DMA、IC 失效。

## 核心要点

### 关键案例

| 案例 | 屏障使用 | 核心知识点 |
|------|----------|-----------|
| 消息传递 | `dmb ishst` + `dmb ishld` | Store-Store + Load-Load 屏障配对 |
| 自旋锁 | `dmb ish` | 获取后+释放前全屏障 |
| DMA | `dsb sy` | DSB 完全停住 |
| IC 失效 | `dsb ish` + `isb` | TLB 刷新后 DSB+ISB |

### 推荐实践

1. 在 QEMU 多核上跑消息传递实验，不加屏障 vs 加屏障对比结果
2. 用 `perf stat` 对比有屏障/无屏障的性能差异
3. 阅读 Linux `arch/arm64/include/asm/barrier.h` 理解 API 实现

## HFT 关联

消息传递案例是 HFT SPSC 无锁队列的原型——在 QEMU 上验证"不加屏障消费者读到旧数据"的现象，可以直观理解 ARM 弱序模型的影响。建议在 Pi5 上做实际测试，因为 QEMU 的内存模型可能比真实硬件更强（QEMU 不完全模拟弱序）。性能对比数据（有屏障 vs 无屏障的延迟差）可以帮助量化屏障开销，在 HFT 系统中做出正确的性能-正确性权衡。

## 自测题

1. **如何在 QEMU 上验证 ARM 弱序内存模型的影响？**

<details>
<summary>答案</summary>

编写消息传递测试：两个核，核 A 写 data 后写 flag，核 B 读 flag 后读 data。
- 不加屏障：多次运行，观察核 B 偶尔读到 data 旧值（flag=1 但 data≠42）
- 加屏障（`dmb ishst` + `dmb ishld`）：核 B 总是读到 data=42

注意：QEMU 可能不完全模拟弱序，建议在真实 Pi5 上测试更可靠。
</details>

2. **`perf stat` 能否测量屏障指令的开销？**

<details>
<summary>答案</summary>

可以间接测量。用 `perf stat` 对比有屏障和无屏障版本的执行时间差异：
```bash
perf stat ./no_barrier_version
perf stat ./with_barrier_version
```
差异主要来自 DMB/DSB 的停顿周期。也可以用 `perf stat -e cycles,inst_retired` 对比 IPC 变化。注意缓存效应可能干扰测量，需要多次运行取平均。
</details>

3. **阅读 Linux `barrier.h` 应该关注什么？**

<details>
<summary>答案</summary>

关注：
1. `smp_mb`/`smp_rmb`/`smp_wmb` 的定义（展开为什么 ARM 指令）
2. `__smp_mb()` vs `mb()` 的区别（SMP vs DMA）
3. `barrier()` 的定义（编译器屏障）
4. `__smp_store_release()` / `__smp_load_acquire()` 是否用 LDAR/STLR
5. 不同 ARM 版本（ARMv8.0 vs 8.1+）的屏障实现差异
</details>

## 参考与延伸

- [§18.3 典型场景](03-typical-scenarios.md) — 各场景的屏障使用
- [§18.7 易错点](07-pitfalls.md) — 屏障使用常见错误
- [Ch19 全章](../../chapter-19-barrier-usage/notes/section-0-本章完整概述.md) — Linux 内核中的真实屏障案例
