# §18.3 典型场景

> **来源：** [Ch18 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

屏障指令的 4 个典型使用场景：消息传递（生产者→消费者）、自旋锁、DMA、TLB 维护。每个场景需要不同类型和位置的屏障。本节给出每个场景的代码和屏障选择依据。

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

| 场景 | 屏障 | 作用域 | 约束 | 原因 |
|------|------|--------|------|------|
| 消息传递（写） | `dmb ishst` | ish | 仅 Store | 只需 Store-Store 有序 |
| 消息传递（读） | `dmb ishld` | ish | 仅 Load | 只需 Load-Load 有序 |
| 自旋锁（获取后） | `dmb ish` | ish | 全部 | 临界区不重排到锁前 |
| 自旋锁（释放前） | `dmb ish` | ish | 全部 | 临界区写在释放前可见 |
| DMA（启动前） | `dsb sy` | sy | 完全停住 | 数据完全写入内存 |
| DMA（完成后） | `dsb sy` | sy | 完全停住 | DMA 写入对 CPU 可见 |
| TLB（刷新后） | `dsb ish` + `isb` | ish | 完全停住+取指 | 等 TLB 完成 + 重取指 |

### 各场景屏障必要性分析

| 场景 | 不加屏障后果 | 为什么选这个屏障 |
|------|------------|----------------|
| 消息传递（写） | 消费者看到 flag=1 但 data 是旧值 | Store-Store 重排 → 需 `ishst` |
| 消息传递（读） | 读 flag=1 但读到 data 旧值 | Load-Load 重排 → 需 `ishld` |
| 自旋锁（获取） | 临界区代码跑到锁外执行 | Load-Store/Store-Store 重排 → 需 `ish` |
| 自旋锁（释放） | 释放先于临界区写可见 | Store-Store 重排 → 需 `ish` |
| DMA（启动前） | DMA 读到不完整数据 | DMB 不够强（不停 CPU）→ 需 `dsb sy` |
| TLB（刷新后） | 用旧 TLB 访问错误地址 | 异步刷新+流水线旧指令 → 需 `dsb` + `isb` |

### 用 LDAR/STLR 优化

```c
// 消息传递 — 用 LDAR/STLR 替代显式 DMB
// 生产者
data = 42;
STLR(flag, 1);       // Store-Release：data 写在 flag 之前可见

// 消费者
while (LDAR(flag) != 1) ;  // Load-Acquire：读 flag 后再读 data
x = data;            // 一定能看到 42

// 自旋锁 — 用 LDAXR/STLXR 替代 LDXR/STXR + DMB
// 获取锁
while (LDAXR_STLXR_lock(&lock)) ;  // 自带 acquire
// 临界区
shared_var = 42;
// 释放锁
STLR(lock, 0);       // 自带 release
```

## HFT 关联

这 4 个场景在 HFT 中都有直接应用。

### HFT 场景对应

| HFT 场景 | 对应模式 | 屏障 | 优化 |
|---------|---------|------|------|
| SPSC 无锁队列 push | 消息传递（写） | `dmb ishst` 或 STLR | 用 STLR 最优 |
| SPSC 无锁队列 pop | 消息传递（读） | `dmb ishld` 或 LDAR | 用 LDAR 最优 |
| 多核订单簿更新 | 自旋锁或 CAS | `dmb ish` | 避免锁，用 SPSC |
| 网卡 DMA 收发 | DMA | `dsb sy` | 用 coherent DMA 省屏障 |
| 进程切换 | TLB 维护 | `dsb ish` + `isb` | 绑核避免切换 |

消息传递模式是 SPSC 无锁队列的核心——生产者写数据后写 index，需要 `dmb ishst` 或 `STLR`。自旋锁在 HFT 中应避免（会导致忙等浪费 CPU），但如果用自旋锁保护短临界区，必须正确加屏障。DMA 场景在网卡收发包中常见。TLB 维护在进程切换时发生，HFT 应避免频繁切换。

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

4. **自旋锁的屏障能否用 `dmb ishst` 替代 `dmb ish`？为什么？**

<details>
<summary>答案</summary>

**不能**。自旋锁需要约束 Load+Store 两种操作：
- 获取锁后：临界区可能有 Load 和 Store，都需要不重排到锁之前 → 需全屏障 `dmb ish`
- 释放锁前：临界区的 Store 需要在释放前可见，但获取锁的 Load 也可能被重排 → 需全屏障

`dmb ishst` 只约束 Store，不约束 Load。如果临界区有 Load 被重排到锁获取之前，可能读到其他核正在修改的数据。
</details>

## 参考与延伸

- [§18.2 三条屏障指令](02-three-barriers.md) — DMB/DSB/ISB 详解
- [§18.4 Acquire/Release](04-acquire-release.md) — 用 LDAR/STLR 替代显式屏障
- [Ch19 §19.5 屏障选择决策树](../../chapter-19-barrier-usage/notes/section-0-本章完整概述.md) — 完整的决策流程
