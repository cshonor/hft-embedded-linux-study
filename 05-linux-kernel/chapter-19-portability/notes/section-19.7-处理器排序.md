## ⑦ 处理器排序 · Processor Ordering

**弱排序（weak ordering）** 架构 — CPU 可 **打乱** load/store 顺序换性能。

| 需求 | 代码 **依赖** 读写绝对顺序时 |
|------|------------------------------|
| 手段 | **内存屏障** — **Ch 10** |

| 宏 | 用途 |
|----|------|
| **`rmb()`** | 读顺序 |
| **`wmb()`** | 写顺序 |
| **`mb()`** | 读写 |

→ SMP 可见性 · 设备 MMIO



<details>
<summary>自测题（点击展开）</summary>

**Q1.** x86 和 ARM64 的内存排序模型有什么区别？对 HFT 有什么影响？

<details><summary>答案</summary>

x86 = TSO（Total Store Order）：store-store 不重排，load-store 不重排，但 store-load 可重排。ARM64 = 弱排序：所有 load/store 都可能重排。影响：ARM64 上需要更多内存屏障（dmb 指令）。HFT 代码用 `smp_store_release/smp_load_acquire`（自带屏障）比裸 `READ_ONCE/WRITE_ONCE` 更安全。跨平台无锁代码必须正确使用屏障。

</details>

</details>
---
