# §18.1 弱序内存模型

> **来源：** [Ch18 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

ARM 是弱序内存模型（Weakly Ordered）——CPU 可以乱序执行、重排访存。x86 是强序（TSO），Store-Store 不重排。弱序模型允许 CPU 更高效执行，但程序员必须显式加屏障保证正确性。

## 核心要点

### 强序 vs 弱序

| 模型 | 代表 | 说明 |
|------|------|------|
| **强序** (Strongly Ordered) | x86（TSO） | Load 不能重排到 Load 前；Store 不能重排到 Store 前 |
| **弱序** (Weakly Ordered) | **ARM**、RISC-V | 几乎所有访存都可重排，除非显式加屏障 |

### 为什么要弱序？

```
// CPU 看到的指令顺序
str x1, [addr_data]    // 1. 写数据
str x2, [addr_flag]    // 2. 写标志

// 另一个核可能先看到 flag 更新，再看到 data 更新！
// （ARM 允许 Store-Store 重排）
```

弱序模型允许 CPU/Cache 乱序执行以提高性能，但程序员必须显式加屏障保证正确性。

### ARM 允许的重排

| 重排类型 | x86 TSO | ARM |
|----------|---------|-----|
| Load-Load | 不重排 | **可重排** |
| Load-Store | 不重排 | **可重排** |
| Store-Store | 不重排 | **可重排** |
| Store-Load | 可重排 | **可重排** |

> ARM 比 x86 弱得多——x86 只允许 Store-Load 重排，ARM 允许所有四种重排。

## HFT 关联

弱序内存模型是 HFT 无锁编程的核心挑战。在 x86 上"碰巧正确"的无锁代码在 ARM 上可能失败——因为 x86 的 TSO 保证了 Store-Store 不重排，但 ARM 不保证。HFT 系统如果跨平台（x86 服务器 + ARM 嵌入式），必须在 ARM 上加显式屏障。SPSC 无锁队列是最典型的案例：生产者写数据后写 flag，在 x86 上不需要屏障（TSO 保证 Store-Store 有序），在 ARM 上必须加 `dmb ishst` 或用 `STLR`（Store-Release）。

## 自测题

1. **ARM 为什么采用弱序内存模型？有什么好处和代价？**

<details>
<summary>答案</summary>

**好处**：允许 CPU 乱序执行、写合并、cache 延迟写回等优化，提高性能和能效。ARM 面向移动/嵌入式场景，功耗敏感，弱序模型让 CPU 有更大优化空间。

**代价**：程序员必须显式加屏障保证多核同步的正确性。忘记加屏障会导致最难调试的并发 bug——代码在 x86 上"碰巧正确"但在 ARM 上随机失败。
</details>

2. **x86 TSO 和 ARM 各允许哪些访存重排？**

<details>
<summary>答案</summary>

- **x86 TSO**：只允许 **Store-Load** 重排，Load-Load/Load-Store/Store-Store 都不重排
- **ARM**：**全部四种**都可重排（Load-Load、Load-Store、Store-Store、Store-Load）

ARM 比 x86 弱得多。x86 上"自然正确"的代码在 ARM 上可能失败。
</details>

3. **写数据后写 flag 的模式，在 x86 和 ARM 上分别需要什么？**

<details>
<summary>答案</summary>

```c
data = 42;      // Store 1
flag = 1;       // Store 2
```

- **x86**：不需要屏障。TSO 保证 Store-Store 不重排，另一个核一定先看到 data=42 再看到 flag=1。
- **ARM**：需要 `dmb ishst`（Store-Store 屏障）或用 `STLR`（Store-Release）。否则 ARM 可能重排 Store 1 和 Store 2，另一个核先看到 flag=1 但 data 还是旧值。
</details>

## 参考与延伸

- [§18.2 三条屏障指令](02-three-barriers.md) — 如何加屏障
- [§18.3 典型场景](03-typical-scenarios.md) — 消息传递等场景的屏障使用
- [§18.4 Acquire/Release](04-acquire-release.md) — LDAR/STLR 自带屏障
