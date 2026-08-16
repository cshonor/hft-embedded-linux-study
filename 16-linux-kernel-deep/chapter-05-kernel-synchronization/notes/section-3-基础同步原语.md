## 3. 基础同步原语

---

### 一、每 CPU 变量 (Per-CPU variables)

- 在 **每个 CPU 上复制一份** 数据结构  
- 本 CPU 只访问自己的副本 → **避免跨核锁**  
- 适合统计计数、per-CPU 缓存等

---

### 二、原子操作 (Atomic operations)

- **读-修改-写** 在 **单条指令**（或不可分割序列）内完成  
- 防止操作中途被中断打断  
- 引用计数、位图等的基础

---

### 三、内存屏障 (Memory barriers)

| 问题 | 屏障作用 |
|------|----------|
| 编译器重排 | 限制编译器优化顺序 |
| CPU/Store buffer 重排 | 保证 **内存操作可见顺序** |

SMP 下锁实现、无锁算法都依赖屏障语义。

---

### 四、本地中断禁用 (Local IRQ disabling)

- **`local_irq_disable()`** — 当前 CPU 上禁止 **可屏蔽中断**  
- 常与 **自旋锁** 联用：
  - 单 CPU：防 ISR 与当前路径 **重入** 同一数据  
  - 多 CPU：还需 spinlock 防其他核并发

| 组合 | 防谁 |
|------|------|
| 仅 spinlock | 其他 CPU / 内核路径 |
| spinlock + local_irq_disable | 再加 **本核 ISR** |

→ 中断嵌套：[Ch 4](../../chapter-04-interrupts-and-exceptions/)

### 常见陷阱

1. 把原子操作当万能锁——原子操作只保证单个操作原子性，不保证多操作组合的原子性
2. 混淆 `atomic_t` 和 `refcount_t`——refcount_t 防溢出（不会从 0 下溢到 -1），atomic_t 会
3. 以为 `smp_mb()` 在所有架构上一样——x86 有较强的内存模型，很多 barrier 是空操作；ARM64 需要真正的 barrier 指令

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** `atomic_t` 和 `refcount_t` 的区别？为什么推荐用 `refcount_t`？

<details><summary>答案</summary>

`atomic_t`：纯原子计数器，无防溢出。`atomic_dec(&v)` 可以从 0 变成 -1（UAF 漏洞）。`refcount_t`：引用计数专用，`refcount_dec()` 在 0 时 WARN + 阻止下溢。6.x 内核中 `task_struct` 的 `usage` 已从 `atomic_t` 改为 `refcount_t`。安全代码应始终用 `refcount_t` 管理生命周期。

</details>

**Q2.** `smp_mb()` / `smp_rmb()` / `smp_wmb()` 分别保证什么？

<details><summary>答案</summary>

`smp_mb()`：全屏障，之前的读写和之后的读写都不可重排。`smp_rmb()`：读屏障，之前的读不可重排到之后的读之后。`smp_wmb()`：写屏障，之前的写不可重排到之后的写之后。x86 上 `smp_rmb()` 是空操作（loads 不重排），`smp_wmb()` 也是空操作（stores 不重排），只有 `smp_mb()` 有 `mfence`。ARM64 上三者都是真实指令。

</details>

**Q3.** HFT 用户态如何利用原子操作避免锁？

<details><summary>答案</summary>

用 `std::atomic<T>` 的无锁操作：① 单生产者单消费者队列：`atomic<size_t> head, tail` + `release`/`acquire` 内存序。② 引用计数：`shared_ptr` 底层是 `atomic` 引用计数。③ 心跳/序列号：`atomic<uint64_t>` + `memory_order_relaxed`。关键是选对内存序：`relaxed`（无屏障）→ `acquire`/`release`（一对屏障）→ `seq_cst`（全屏障，最安全最慢）。

</details>

</details>

---

← [2. 内核抢占](./section-2-内核抢占.md) · 下一节 [4. 自旋锁](./section-4-自旋锁.md)
