## ④ 信号量 · Semaphores

**睡眠锁**：拿不到时任务 **睡眠等待**，不空转 CPU。可实现 **互斥（计数=1）** 或 **资源池（计数>1）**。

| 属性 | 说明 |
|------|------|
| 争用 | **睡眠**（可被调度走） |
| 上下文 | **仅进程上下文** — ISR/softirq **禁止** `down` 睡眠 |
| 持有时间 | 可较长（相对 spinlock） |

#### 操作直觉

| 操作 | 含义 |
|------|------|
| **`down` / `down_interruptible`** | P 操作 — 计数减；不够则睡 |
| **`up`** | V 操作 — 计数加；唤醒等待者 |
| **`down_trylock`** | 不睡，失败立即返回 |

```
计数初始 = 3（三个缓冲槽）
任务 A down → 2
任务 B down → 1
任务 C down → 0
任务 D down → 睡眠……
A 用完 up → 1，唤醒 D
```

#### 与 mutex 的关系

| | semaphore | mutex（10.5） |
|--|-----------|----------------|
| 计数 | 可 >1 | **二值**，专为互斥 |
| 历史 | 更老、更通用 | **新互斥代码首选** |
| 所有者 | 语义较弱 | 有明确所有者、严格规则 |

**驱动场景：** 限制同时进入某慢路径的任务数、等待硬件「有空槽」。

**HFT：** 用户态 `sem_wait` 同类；热路径 **避免** 睡眠锁 — 唤醒延迟直接进尾延迟。控制面、初始化、错误路径可以用。

→ [10.5 mutex](./section-10.5-互斥体.md) · [4.4 休眠唤醒](../chapter-04-process-scheduling/notes/section-4.4-休眠与唤醒.md)

### 常见陷阱

1. 混淆 semaphore 和 mutex——mutex 有归属（只有持有者能解锁），semaphore 无归属
2. 以为 counting semaphore 常用于内核——内核中大多用 mutex，semaphore 只在特殊场景
3. 在中断上下文中调用 down()——down() 会睡眠，中断只能用 down_trylock()

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** semaphore 和 mutex 的核心区别？

<details><summary>答案</summary>

mutex：① 有归属（owner 字段），只有持锁者能 unlock。② 支持优先级继承（rt_mutex）。③ 支持 lockdep。semaphore：① 无归属，任何人可以 up()。② 初始值可 >1（计数信号量）。③ 无优先级继承。内核新代码推荐 mutex，semaphore 只在需要计数语义或无归属场景使用。

</details>

**Q2.** counting semaphore 在什么场景下有用？

<details><summary>答案</summary>

① 限制并发数：如限制同时打开的文件数（初始值=最大并发数）。② 生产者-消费者：semaphore 计数 = 队列中可用元素数，消费者 down() 取数据，生产者 up() 放数据。③ 资源池：初始值=池大小，获取资源 down()，释放 up()。但在内核中，这些场景更常用 kfifo + waitqueue 或 mempool。

</details>

**Q3.** HFT 中 semaphore 的用户态对应物？

<details><summary>答案</summary>

① `std::counting_semaphore<N>`（C++20）：计数信号量。② `sem_t`（POSIX）：进程间或线程间信号量。③ `std::binary_semaphore` = mutex 的近似。HFT 热路径避免 semaphore（有 futex 开销），用无锁队列代替生产者-消费者模式。非热路径可以用 semaphore 做资源限流。

</details>

</details>


> ↔ [ULK Ch5 §6 信号量与完成变量](../../../../20-linux-kernel-deep/chapter-05-kernel-synchronization/notes/section-6-信号量与完成变量.md)
---
