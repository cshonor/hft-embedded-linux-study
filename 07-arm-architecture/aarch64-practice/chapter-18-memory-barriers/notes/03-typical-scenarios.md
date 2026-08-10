# §18.3 典型场景

> **来源：** [Ch18 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

屏障指令的 4 个典型使用场景：消息传递（生产者→消费者）、自旋锁、DMA、TLB 维护。每个场景需要不同类型和位置的屏障。

## 核心要点

### 场景1：消息传递（生产者→消费者）

```c
// 生产者
data = 42;
dmb ishst;           // 保证 data 写在 flag 之前可见
flag = 1;

// 消费者
while (flag != 1) ;
dmb ishld;           // 保证读 flag 后再读 data
x = data;            // 一定能看到 42
```

### 场景2：自旋锁

```c
// 获取锁
while (ldxr_stxr_lock(&lock)) ;
dmb ish;             // 获取锁后，保证后续访存不重排到锁之前

// 临界区
shared_var = 42;

// 释放锁
dmb ish;             // 保证临界区写在释放之前可见
lock = 0;
```

### 场景3：DMA

```c
// 内存→设备（DMA 读）
write_descriptor();
dsb sy;              // 确保描述符写入对 DMA 可见
start_dma();

// 设备→内存（DMA 写）
wait_dma_complete();
dsb sy;              // 确保 DMA 写入对 CPU 可见
read_data();
```

### 场景4：TLB 维护

```asm
msr TTBR0_EL1, x0   // 切换页表
tlbi alle1           // 刷新 TLB
dsb ish              // 等 TLB 刷新完成
isb                  // 重新取指
```

### 屏障选择总结

| 场景 | 屏障 | 原因 |
|------|------|------|
| 消息传递（写） | `dmb ishst` | 只需 Store-Store 有序 |
| 消息传递（读） | `dmb ishld` | 只需 Load-Load 有序 |
| 自旋锁 | `dmb ish` | 需要 Load+Store 全屏障 |
| DMA | `dsb sy` | 需要完全停住 + 全系统可见 |
| TLB | `dsb ish` + `isb` | 需要停住 + 重新取指 |

## HFT 关联

这 4 个场景在 HFT 中都有直接应用。消息传递模式是 SPSC 无锁队列的核心——生产者写数据后写 index，需要 `dmb ishst` 或 `STLR`。自旋锁在 HFT 中应避免（会导致忙等浪费 CPU），但如果用自旋锁保护短临界区，必须正确加屏障。DMA 场景在网卡收发包中常见。TLB 维护在进程切换时发生，HFT 应避免频繁切换。理解每个场景的屏障选择可以避免过度使用屏障（性能损失）或不足使用屏障（正确性问题）。

## 自测题

1. **消息传递场景中，生产者和消费者各需要什么屏障？**

<details>
<summary>答案</summary>

- **生产者**：写数据后 `dmb ishst`（Store-Store 屏障，Inner Shareable）再写 flag。保证 data 写在 flag 之前对其他核可见。
- **消费者**：读 flag 后 `dmb ishld`（Load-Load 屏障）再读 data。保证读 flag=1 后再读 data，能看到生产者写的值。

用 `ishst`/`ishld` 而不是 `sy` 是因为只需 Store-Store 或 Load-Load 约束，不需要全屏障。
</details>

2. **DMA 场景为什么必须用 DSB 而不是 DMB？**

<details>
<summary>答案</summary>

DMB 只保证访存顺序但 CPU **不停**。如果用 DMB，`start_dma()` 可能在 `write_descriptor()` 的数据还没完全写入内存时就执行 → DMA 读到不完整的描述符。DSB **完全停住 CPU**，确保 `write_descriptor()` 写入完成后才执行 `start_dma()`。DMA 场景需要保证"完成"而非只是"顺序"，必须用 DSB。
</details>

3. **TLB 维护场景中 `dsb ish` 和 `isb` 分别做什么？**

<details>
<summary>答案</summary>

- `dsb ish`：等待 `tlbi alle1`（TLB 刷新）在所有 Inner Shareable CPU 上**完成**。TLB 刷新是异步的，需要 DSB 等待。
- `isb`：冲刷本核**流水线**。流水线中可能有使用旧 TLB 映射的指令，ISB 确保后续指令用新 TLB 重新翻译。

两者缺一不可：DSB 保证 TLB 刷新完成，ISB 保证流水线同步。
</details>

## 参考与延伸

- [§18.2 三条屏障指令](02-three-barriers.md) — DMB/DSB/ISB 详解
- [§18.4 Acquire/Release](04-acquire-release.md) — 用 LDAR/STLR 替代显式屏障
- [Ch19 §19.5 屏障选择决策树](../../chapter-19-barrier-usage/notes/section-0-本章完整概述.md) — 完整的决策流程
