# §18.1 弱序内存模型

> **来源：** [Ch18 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

ARM 是弱序内存模型（Weakly Ordered）——CPU 可以乱序执行、重排访存。x86 是强序（TSO），Store-Store 不重排。弱序模型允许 CPU 更高效执行，但程序员必须显式加屏障保证正确性。本节分析强弱内存模型的区别、ARM 允许的重排类型、以及跨平台编程的陷阱。

## 核心要点

### 强序 vs 弱序

| 模型 | 代表 | Store-Store | Load-Load | Store-Load | 说明 |
|------|------|------------|-----------|------------|------|
| **强序** (TSO) | x86 | 不重排 | 不重排 | **可重排** | 只有 Store-Load 可重排 |
| **弱序** (Weakly Ordered) | **ARM**、RISC-V | **可重排** | **可重排** | **可重排** | 所有四种都可重排 |
| **严格顺序** | None | 不重排 | 不重排 | 不重排 | 每条访存指令完全按序 |

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

| 重排类型 | x86 TSO | ARM | 说明 |
|----------|---------|-----|------|
| Load-Load | 不重排 | **可重排** | 两个读可以交换顺序 |
| Load-Store | 不重排 | **可重排** | 读后面的写可提前 |
| Store-Store | 不重排 | **可重排** | 两个写可以交换顺序 |
| Store-Load | 可重排 | **可重排** | x86 也允许这种重排 |

> ARM 比 x86 弱得多——x86 只允许 Store-Load 重排，ARM 允许所有四种重排。

### 重排的原因

| 重排来源 | 说明 | 示例 |
|---------|------|------|
| CPU 乱序执行 | 分支预测后的指令提前执行 | Load 在前一条 Store 等待 cache 时提前 |
| 写缓冲合并 | 连续 Store 到不同地址可在写缓冲中重排 | Store A 和 Store B 顺序在写缓冲中交换 |
| Cache 延迟写回 | Write-Back cache 不立即写内存 | 其他核看到内存旧值 |
| 编译器优化 | 编译器在编译时重排指令 | 循环中不变的 Load 提前到循环外 |

### 消息传递问题（Message Passing）

```c
// 生产者
data = 42;       // Store 1
flag = 1;        // Store 2

// 消费者
while (flag != 1) ;
x = data;        // x 可能不是 42！
```

| 平台 | data=42 先于 flag=1 可见？ | 需要屏障？ |
|------|--------------------------|-----------|
| x86 (TSO) | 保证（Store-Store 不重排） | 不需要 |
| ARM (弱序) | **不保证**（Store-Store 可重排） | **需要** `dmb ishst` |
| ARM + STLR | 保证（STLR 自带 release） | 不需要（用 STLR 替代 STR） |

### 观察一致性问题

```
CPU0 执行:              CPU1 观察:
Store data=42           → 可能先看到 flag=1
Store flag=1            → 后看到 data=42

原因: ARM 允许 Store-Store 重排
       写缓冲可能先提交 flag（不同 cache line）
```

## HFT 关联

弱序内存模型是 HFT 无锁编程的核心挑战。在 x86 上"碰巧正确"的无锁代码在 ARM 上可能失败——因为 x86 的 TSO 保证了 Store-Store 不重排，但 ARM 不保证。

### 跨平台 HFT 陷阱

```c
// x86 上正确，ARM 上错误的 SPSC 队列
void producer_bad(T val) {
    buffer[w] = val;        // Store 1
    write_idx++;            // Store 2
    // x86: Store-Store 不重排 → 消费者安全
    // ARM: Store-Store 可重排 → 消费者可能看到新 index 但旧 buffer！
}

// ARM 正确版本
void producer_good(T val) {
    buffer[w] = val;        // Store 1
    dmb ishst;              // Store-Store 屏障
    write_idx++;            // Store 2
}

// 最优版本（用 STLR）
void producer_best(T val) {
    buffer[w] = val;        // 普通写
    // STLR 自带 release 语义
    __asm__ volatile("stlr %w0, %1" :: "r"(w+1), "Q"(write_idx));
}
```

HFT 系统如果跨平台（x86 服务器 + ARM 嵌入式），必须在 ARM 上加显式屏障。SPSC 无锁队列是最典型的案例：生产者写数据后写 flag，在 x86 上不需要屏障（TSO 保证 Store-Store 有序），在 ARM 上必须加 `dmb ishst` 或用 `STLR`（Store-Release）。

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

4. **ARM 弱序模型的访存重排有哪些来源？编译器屏障和硬件屏障分别管哪个？**

<details>
<summary>答案</summary>

重排来源：CPU 乱序执行、写缓冲合并、Cache 延迟写回、编译器优化。

- **编译器屏障**（`barrier()` / `volatile`）：阻止编译器在编译时重排指令。对 CPU 乱序无效。
- **硬件屏障**（`DMB`/`DSB`）：阻止 CPU 在运行时乱序执行和写缓冲重排。对编译器重排无效。

两者需要配合使用——只有硬件屏障，编译器可能在编译时就把 Store 重排了；只有编译器屏障，CPU 可能在运行时重排。
</details>

## 参考与延伸

- [§18.2 三条屏障指令](02-three-barriers.md) — 如何加屏障
- [§18.3 典型场景](03-typical-scenarios.md) — 消息传递等场景的屏障使用
- [§18.4 Acquire/Release](04-acquire-release.md) — LDAR/STLR 自带屏障
