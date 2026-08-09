# Ch18 完整总结 · 内存屏障指令

> **《ARM64体系结构编程与实践》** · 奔跑吧Linux社区 · **精读** · HFT 无锁同源  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md) · [Pi5 适配](../../PI5-ADAPT.md)

---

## 本章定位

ARM 是**弱序内存模型**（Weakly Ordered）——CPU 可以乱序执行、重排访存。内存屏障指令（DMB/DSB/ISB）控制访存顺序，是多核同步的基础。

> **HFT 关联**：无锁队列、SPSC ring buffer 必须用屏障保证生产者写数据 → 写可见的顺序，否则消费者看到更新了 index 但数据还没写。

---

## 18.1 弱序内存模型 ⭐

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

---

## 18.2 三条屏障指令 ⭐

| 指令 | 全称 | 行为 | 强度 |
|------|------|------|------|
| **DMB** | Data Memory Barrier | 保证屏障前的访存完成后，屏障后的访存才**对外可见**。CPU 可继续执行非访存指令。 | 中 |
| **DSB** | Data Synchronization Barrier | 比 DMB 更强：屏障前的访存完成后，**才执行后续任何指令**（完全停住）。 | 强 |
| **ISB** | Instruction Synchronization Barrier | 冲刷流水线，保证后续指令**重新取指**。用于修改系统寄存器/代码后。 | 指令侧 |

### DMB vs DSB 区别

```
// DMB：访存有序，但 CPU 不停
str x1, [data]     // Store A
dmb sy             // 屏障
str x2, [flag]     // Store B 必须在 A 之后对外可见
add x3, x4, x5     // CPU 可以先执行这条（非访存）

// DSB：完全停住
str x1, [data]     // Store A
dsb sy             // 等 A 完成，且停住
add x3, x4, x5     // 必须等 DSB 完成才能执行
```

### 屏障作用域

| 后缀 | 作用域 |
|------|--------|
| `sy` | Full system（所有可观察者） |
| `ish` | Inner Shareable（同一簇 CPU） |
| `osh` | Outer Shareable（包括 DMA） |
| `nsh` | Non-shareable（仅当前 CPU） |
| `st` | 仅 Store（不约束 Load） |

```asm
dmb sy      // 全屏障：Load+Store，全系统
dmb ishst   // 仅 Store 屏障，Inner Shareable
dsb sy      // 最强：全停，全系统
isb         // 指令同步
```

---

## 18.3 典型场景 ⭐

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

---

## 18.4 Acquire / Release 语义

C++11 `std::atomic` 的内存序对应 ARM 屏障：

| C++ 内存序 | ARM 实现 | 含义 |
|-----------|----------|------|
| `memory_order_relaxed` | 无屏障 | 只保证原子性，不保证顺序 |
| `memory_order_acquire` | Load + `dmb ishld` | 后续读不能重排到此 Load 前 |
| `memory_order_release` | `dmb ishst` + Store | 前面写不能重排到此 Store 后 |
| `memory_order_seq_cst` | `dmb ish` + ... | 全序（最强） |

### LDAR / STLR（自带屏障）

| 指令 | 语义 |
|------|------|
| `LDAR` | Load-Acquire：后续访存不能重排到此 Load 前 |
| `STLR` | Store-Release：前面访存不能重排到此 Store 后 |

```c
// 等价于 acquire load
// C++: std::atomic<int> flag;
// int v = flag.load(std::memory_order_acquire);
// ↓ 编译为
ldar w0, [flag_addr]
```

> **LDAR/STLR 比显式 DMB 更高效**：CPU 知道语义意图，可以更精确地优化。

---

## 18.5 Linux 内核屏障 API

| API | 展开为 | 用途 |
|-----|--------|------|
| `smp_mb()` | `dmb ish` | 全屏障（SMP） |
| `smp_rmb()` | `dmb ishld` | 读屏障 |
| `smp_wmb()` | `dmb ishst` | 写屏障 |
| `mb()` | `dsb sy` | 全屏障（含 DMA） |
| `rmb()` | `dsb ld` | 读屏障（含 DMA） |
| `wmb()` | `dsb st` | 写屏障（含 DMA） |
| `barrier()` | 编译器屏障 | 只阻止编译器重排，不加硬件屏障 |

> `smp_*` 用于 CPU 间同步；不带 `smp_` 的用于 CPU 与 DMA 同步。

---

## 18.6 实验要点

本章以案例分析为主。关键案例：消息传递、自旋锁、邮箱传递、DMA、IC 失效。

---

## 18.7 易错点清单

1. **忘加屏障** → 乱序导致数据不一致（最难 debug 的 bug）。
2. **DMB 和 DSB 混用** → DMB 不停 CPU，DMA 场景可能不够强。
3. **作用域选错** → `nsh` 只管本核，多核场景要用 `ish`。
4. **编译器重排** → 硬件屏障不阻止编译器重排，需要 `barrier()` 或 `volatile`。
5. **过度使用屏障** → 性能下降。能用 acquire/release 就不用 seq_cst。

---

## 书中思考题（自测）

1. ARM 为什么是弱序内存模型？有什么好处和代价？
2. DMB 和 DSB 的区别？什么时候必须用 DSB？
3. 消息传递场景中，生产者和消费者各需要什么屏障？
4. LDAR/STLR 和 DMB 相比有什么优势？
5. `smp_mb()` 和 `mb()` 的区别？

**参考答案：**

1. 弱序允许 CPU 乱序执行提高性能。代价是程序员必须**显式加屏障**保证正确性。  
2. DMB 约束访存顺序但**CPU 不停**；DSB **完全停住**等访存完成。DMA/TLB 场景必须用 DSB。  
3. 生产者：写数据后 `dmb ishst` 再写 flag。消费者：读 flag 后 `dmb ishld` 再读数据。  
4. LDAR/STLR **自带屏障语义**，CPU 知道意图可更精确优化，通常比显式 DMB **更高效**。  
5. `smp_mb()` = `dmb ish`（CPU 间）；`mb()` = `dsb sy`（含 DMA，更强）。

---

上一章 [Ch17 TLB管理](../../chapter-17-tlb-management/) · 下一章 [Ch19 屏障使用](../../chapter-19-barrier-usage/) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md)
