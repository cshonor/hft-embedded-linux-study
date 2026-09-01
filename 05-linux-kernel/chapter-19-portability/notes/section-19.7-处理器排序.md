## ⑦ 处理器排序 · Processor Ordering

**弱排序（weak ordering）** 架构 — CPU 可 **打乱** load/store 顺序换性能。

| 需求 | 代码 **依赖** 读写绝对顺序时 |
|------|------------------------------|
| 手段 | **内存屏障** — **Ch 10** |

> **两层重排，都要防：**
> ```
> ① 编译器重排（编译期）—— 优化器认为无关就换顺序   → 用 barrier() / volatile / READ_ONCE
> ② CPU 重排（运行期）  —— 弱序 CPU 真的乱序执行     → 用 smp_mb() / dmb / mfence
> ```

---

### `mb()` vs `smp_mb()`：一个字之差，语义完全不同

```c
/* include/asm-generic/barrier.h — v6.6 原文 */

/* 硬件屏障：总是生效（用于 MMIO / 设备） */
#define mb()	do { kcsan_mb(); __mb(); } while (0)      /* :30 */
#define rmb()	do { kcsan_rmb(); __rmb(); } while (0)    /* :34 */
#define wmb()	do { kcsan_wmb(); __wmb(); } while (0)    /* :38 */

/* SMP 屏障：只在多核时生效 */
#define smp_mb()	do { kcsan_mb(); __smp_mb(); } while (0)   /* :99 */

/* ↓↓↓ 关键：单核（!CONFIG_SMP）下的定义 ↓↓↓ */
#else	/* !CONFIG_SMP */
#ifndef smp_mb
#define smp_mb()	barrier()      /* ← 只剩编译器屏障，没有指令！ */
#endif
#ifndef smp_rmb
#define smp_rmb()	barrier()
#endif
#ifndef smp_wmb
#define smp_wmb()	barrier()
#endif
#endif	/* CONFIG_SMP */
```

| 宏 | SMP 上 | UP 上 | 用途 |
|----|--------|-------|------|
| **`mb()` / `rmb()` / `wmb()`** | 真指令（`mfence`/`dmb`） | **仍是真指令** | **设备 MMIO**、与硬件交互 |
| **`smp_mb()` / `smp_rmb()` / `smp_wmb()`** | 真指令 | **仅 `barrier()`**（编译器屏障） | **CPU 之间**的顺序（普通内存） |

> **为什么这么分？** 单核上不存在"另一个核看到乱序"的问题——
> 唯一的观察者就是这个 CPU，而它自己看到的结果**逻辑上一定与程序序一致**（自一致性保证）。
> 所以 SMP 屏障在 UP 上退化成编译器屏障就够了，**省下几十个周期的 fence 指令**。
>
> **排障启示：** 一个只在 SMP 上暴露的并发 bug，在单核编译的内核上**完全看不出来**。
> 这也是"按最坏情况写"的又一个例证。

---

### 屏障工具箱（从强到弱）

| 工具 | 作用 | 代价 |
|------|------|------|
| **`smp_mb()`** | 全屏障（读写都不跨） | 最贵（x86 上 `mfence`，~20-100 周期） |
| `smp_rmb()` | 只保证**读**不越过 | 读侧较便宜 |
| `smp_wmb()` | 只保证**写**不越过 | **最便宜**（x86 上甚至无指令） |
| **`smp_store_release(&x, v)`** | 写 + 之前的所有读写不能越过它 | 与 `acquire` 配对 |
| **`smp_load_acquire(&x)`** | 读 + 之后的所有读写不能越过它 | 与 `release` 配对 |
| **`READ_ONCE(x)` / `WRITE_ONCE(x, v)`** | **不是屏障**，只保证"一次性读/写+不优化掉" | 几乎免费 |
| `barrier()` | 只挡**编译器**重排 | 零指令 |
| `dma_rmb()` / `dma_wmb()` | 设备与 CPU 之间（弱序架构上弱于 `mb`） | 中等 |

**推荐用法：优先 acquire/release，而不是裸 smp_mb。**

| 对比 | 说明 |
|------|------|
| `smp_mb()` 双向挡 | 两侧都挡 → 最保守，也最贵 |
| `release` / `acquire` **单向**挡 | 只挡"生产者在写之前完成所有准备"和"消费者在读之后才用数据" → **语义精确、开销小**（ARM64 上用 `stlr`/`ldar`，x86 上几乎免费） |

---

### x86 (TSO) vs ARM64 (弱序)：哪些重排被允许

| 重排类型 | x86 (TSO) | ARM64 |
|---------|-----------|-------|
| Load → Load | ❌ 不允许 | ✅ **允许** |
| Load → Store | ❌ 不允许 | ✅ **允许** |
| **Store → Store** | ❌ 不允许 | ✅ **允许** |
| **Store → Load** | ✅ **允许**（唯一漏洞） | ✅ **允许** |

> **结论：x86 上只有 Store-Load 一种重排**，所以绝大多数无锁代码在 x86 上"碰巧正确"。
> 迁到 ARM64 后四类重排全部可能发生，**那些"碰巧正确"的代码就开始出错**。
> 这就是为什么同一份无锁队列代码在 x86 上跑几个月没事，到 ARM64（如 AWS Graviton、树莓派 5）上就崩。

> **Store-Load 是 x86 上唯一需要 `smp_mb()` 的场景**，也就是经典的
> [Dekker / Peterson 互斥算法](../../chapter-09-kernel-sync-intro/notes/section-9.3-并发的原因.md) 会失败的原因。

---

### 实战：SPSC 无锁队列的正确写法

这正是 [Ch 6.3 讲的 kfifo](../../chapter-06-kernel-data-structures/notes/section-6.3-队列.md) 背后的模式：

```c
/* 生产者（单写者） */
WRITE_ONCE(slot[tail].data, value);
smp_wmb();                            /* ① 保证数据先于索引可见 */
WRITE_ONCE(q->tail, tail + 1);        /* ② 发布（publish）        */

/* 消费者（单读者） */
tail = READ_ONCE(q->tail);
if (tail != head) {
	smp_rmb();                        /* ③ 保证索引先于数据被读 */
	value = READ_ONCE(slot[head].data);
	WRITE_ONCE(q->head, head + 1);
}
```

| 屏障 | 缺了会怎样 |
|------|-----------|
| ① `smp_wmb()`（生产者侧） | ARM64 上"索引先更新、数据还没写完"→ 消费者读到**半截数据** |
| ③ `smp_rmb()`（消费者侧） | ARM64 上"先去读数据、后读索引"→ 读到**旧数据/未完成的数据** |

> 用 acquire/release 写更简洁（且在现代 ISA 上更高效）：
> ```c
> smp_store_release(&q->tail, tail + 1);      /* 生产者发布 */
> tail = smp_load_acquire(&q->tail);          /* 消费者获取 */
> ```

→ SMP 可见性 · 设备 MMIO · [Ch 10.10 排序和屏障](../../chapter-10-sync-methods/notes/section-10.10-排序和屏障.md) · [Ch 6.3 队列](../../chapter-06-kernel-data-structures/notes/section-6.3-队列.md)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** x86 和 ARM64 的内存排序模型有什么区别？对 HFT 有什么影响？

<details><summary>答案</summary>

x86 = TSO（Total Store Order）：store-store 不重排，load-store 不重排，但 store-load 可重排。ARM64 = 弱排序：所有 load/store 都可能重排。影响：ARM64 上需要更多内存屏障（dmb 指令）。HFT 代码用 `smp_store_release/smp_load_acquire`（自带屏障）比裸 `READ_ONCE/WRITE_ONCE` 更安全。跨平台无锁代码必须正确使用屏障。

</details>

**Q2.** `mb()` 和 `smp_mb()` 有什么区别？为什么在单核（`!CONFIG_SMP`）内核上 `smp_mb()` 退化成 `barrier()` 是**正确**的而不是偷懒？

<details><summary>答案</summary>

源码（`include/asm-generic/barrier.h`，v6.6）：

```c
#define mb()      do { kcsan_mb(); __mb(); } while (0)      /* :30 总是真指令 */
#ifdef CONFIG_SMP
#define smp_mb()  do { kcsan_mb(); __smp_mb(); } while (0)  /* :99 */
#else
#define smp_mb()  barrier()                                 /* :113 只剩编译器屏障 */
#endif
```

| | 保证对象 | UP 上是否发指令 |
|---|---|---|
| `mb()` | **CPU ↔ 设备（MMIO / DMA）** | **是**（设备不受"只有一颗 CPU"影响） |
| `smp_mb()` | **CPU ↔ CPU** | **否**，只剩 `barrier()` |

UP 上退化的正确性论证：单核只有一个执行流在做乱序发射，而乱序执行的**硬件承诺**是"单核语义不被破坏"——CPU 自己看自己的读写顺序永远是程序序（self-consistency）。所以 UP 上唯一还能打乱可观测顺序的是**编译器**把它重排到寄存器分配/指令调度之后，而 `barrier()` 恰好只管这一层。**不为不存在的场景付费**：SMP 屏障（x86 的 `mfence` 约 20~30 周期）在 UP 上是纯浪费。

三个实践结论：
1. 写**驱动**（访问 MMIO 寄存器）必须用 `mb()` / `dma_wmb()`，**不能**用 `smp_mb()` —— 否则在 UP 内核上设备会看到乱序的寄存器写，这是极难复现的 bug。
2. 写**无锁数据结构**（CPU 之间）用 `smp_mb()` 系列 —— 在 UP 上自动免费，在 SMP 上自动生效。
3. 不要因为"我机器是多核"就以为 `smp_mb()` 一定发指令：编译内核时 `CONFIG_SMP=n` 就全部消失，这正是**测试环境和生产环境配置不一致**导致的经典事故。

</details>

**Q3.** 团队把一个 SPSC 无锁队列从 x86_64 迁到 ARM64，压测几个月都正常，上 ARM64 后立刻偶发数据损坏。代码是：

```c
/* 生产者 */
slot[tail].data = value;
WRITE_ONCE(q->tail, tail + 1);        /* 缺了 smp_wmb() */
/* 消费者 */
tail = READ_ONCE(q->tail);
if (tail != head) {
    value = slot[head].data;          /* 缺了 smp_rmb() */
    WRITE_ONCE(q->head, head + 1);
}
```

请用 x86 TSO 与 ARM64 弱排序的差异解释"为什么 x86 上碰巧正确"，并给出最小修法。

<details><summary>答案</summary>

**为什么 x86 上碰巧正确**

x86 的 TSO 只开放**一类**重排：**Store→Load**（写缓冲导致的"自己的写还没被别人看到，就先读了别人的值"）。而这段代码的危险重排是：

- 生产者侧：`store data` 与 `store tail` 两条 **Store→Store** —— TSO **禁止**重排 → 消费者看到新 tail 时，data 一定已经进 store buffer 并对其他核可见 → 正确。
- 消费者侧：`load tail` 与 `load data` 两条 **Load→Load** —— TSO **禁止**重排 → 看到新 tail 后不会去读旧 data → 正确。

也就是说，**这段代码恰好只依赖 x86 不重排的那三类**，唯一被 x86 放开的 Store→Load 组合它没用到。所以 x86 上缺屏障也"一直没问题"——这正是最危险的一类 bug：**正确性依赖平台，而不是依赖代码**。

> 反例：把消费者改成"先写 `head` 再读 `tail`"（Store→Load），x86 上立刻就崩。

**ARM64 上为什么崩**

ARM64 是弱排序，四类重排（Load-Load / Load-Store / Store-Store / Store-Load）**全部允许**（只要不碰地址依赖）。于是：

| 缺失的屏障 | ARM64 上允许的重排 | 后果 |
|---|---|---|
| 生产者 `smp_wmb()` | Store-Store 重排：`tail` 先于 `data` 可见 | 消费者读到**未初始化的槽位**（半截数据 / 全 0 / 上一个消息的残留） |
| 消费者 `smp_rmb()` | Load-Load 重排：先投机读 `data` 再读 `tail` | 读到**旧值**，然后 head 前进 → 消息重复或错序 |

两者都是**概率性**的（取决于 store buffer 排空时机、乱序窗口宽度），所以表现为"偶发损坏、压力越大越频繁"——最难查的那一类。

**最小修法（两种，推荐后者）**

1. 补裸屏障：
   ```c
   /* 生产者 */ slot[tail].data = value;
   smp_wmb();                              /* 数据先于索引可见 */
   WRITE_ONCE(q->tail, tail + 1);
   /* 消费者 */ tail = READ_ONCE(q->tail);
   if (tail != head) {
       smp_rmb();                          /* 索引先于数据被读 */
       value = slot[head].data;
       WRITE_ONCE(q->head, head + 1);
   }
   ```

2. **推荐**：改用 acquire/release，语义更明确且在 ARM64 上只生成单向屏障（比全向 `dmb ish` 便宜）：
   ```c
   smp_store_release(&q->tail, tail + 1);  /* 生产者：release，前面的写不会越过它 */
   tail = smp_load_acquire(&q->tail);      /* 消费者：acquire，后面的读不会越过它 */
   ```

**给 HFT 的教训**：无锁代码的正确性必须来自**代码里的屏障**，不能来自"某平台上跑过"。可移植的做法是
① 一律用 `smp_store_release/smp_load_acquire` 表达"发布/消费"关系；
② 用 `READ_ONCE/WRITE_ONCE` 消灭编译器优化引起的撕裂；
③ 在 CI 里**同时**跑 x86_64 和 ARM64（或用 `herd7`/`litmus` 做内存模型形式化验证），而不是只在一个平台上压测。

</details>

</details>
---
