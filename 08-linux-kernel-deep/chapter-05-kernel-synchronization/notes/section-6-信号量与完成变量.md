## 6. 信号量 (Semaphores) 与完成变量 (Completions)

---

### 一、信号量

与自旋锁不同：资源不可用时，当前进程 **挂起睡眠 (blocking wait)**，资源就绪后 **唤醒**。

| 变种 | 说明 |
|------|------|
| 计数信号量 | 允许多个持有者（有限资源池） |
| **读/写信号量** | 多读单写 — 提高读并发 |

适合：**仅被异常/系统调用路径访问**、临界区可能较长、**可以睡眠** 的场景。

→ 等待队列：[Ch 3](../chapter-03-processes/notes/section-4-组织与查找.md)

---

### 二、完成变量 (Completions)

- 类似信号量，专用于 **「一件事完成了」** 的同步  
- 解决 SMP 上 **临时信号量动态分配/释放** 的竞态  
- 典型：`complete()` / `wait_for_completion()`

---

### 三、自旋锁 vs 信号量（性能直觉）

| 维度 | 自旋锁 | 信号量 |
|------|--------|--------|
| 等待成本 | 烧 CPU | 让出 CPU |
| 临界区长度 | 必须短 | 可较长 |
| 上下文 | 中断里 **只能用 spinlock** | 中断里 **不能** sleep → 不用 semaphore |
| HFT 热路径 | 首选（短临界区） | 一般避开 |

### 常见陷阱

1. 混淆 `semaphore` 和 `mutex`——mutex 有归属（只有持有者能解锁），semaphore 无归属
2. 以为 `completion` 和 `semaphore` 等价——completion 是一次性信号（complete 后不可复用），semaphore 可重复
3. 在中断上下文中调用 `down()`——`down()` 会睡眠，中断上下文只能用 `down_trylock()`

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** `mutex` 和 `semaphore` 的关键区别？

<details><summary>答案</summary>

mutex：① 有归属（`task_struct *owner`），只有持锁者能 `mutex_unlock()`。② 支持优先级继承（`rt_mutex`，防优先级反转）。③ 支持 `lockdep` 调试。semaphore：① 无归属，任何人可以 `up()`。② 初始值可 >1（计数信号量）。③ 无优先级继承。内核新代码推荐用 `mutex`，`semaphore` 只在需要计数语义时使用。

</details>

**Q2.** `completion` 为什么比 `semaphore` 更适合「等待一次性事件」？

<details><summary>答案</summary>

① completion 语义清晰：`init_completion()` → `wait_for_completion()` → `complete()`，只用于一次性通知。② 防止误用：semaphore 可被多次 `up()`，completion 的 `complete()` 通常只调一次。③ 支持超时：`wait_for_completion_timeout()`。④ 支持中断可中断：`wait_for_completion_interruptible()`。驱动初始化等待硬件就绪是最典型场景。

</details>

**Q3.** HFT 中如何避免 `mutex`/`semaphore` 引起的延迟？

<details><summary>答案</summary>

① 避免在热路径上持锁——用无锁数据结构或 per-CPU 变量。② 如果必须持锁，用 `spinlock_t` + 短临界区（<1us），避免 mutex 的调度开销。③ 用 `futex` 替代 `pthread_mutex`（减少内核态往返）。④ `rt_mutex` 的优先级继承防优先级反转——高优先级交易线程不会被低优先级线程阻塞的锁卡住。⑤ `perf lock` 分析锁等待时间。

</details>

</details>

---

← [5. 顺序锁与 RCU](./section-5-顺序锁与RCU.md) · 下一节 [7. 选型与实例](./section-7-选型与实例.md)
> ↔ [LKD Ch10 §10.5 互斥体](../../../07-linux-kernel/00_Book_3rd_Notes/chapter-10-sync-methods/notes/section-10.5-互斥体.md)
> ↔ [LKD Ch10 §10.4 信号量](../../../07-linux-kernel/00_Book_3rd_Notes/chapter-10-sync-methods/notes/section-10.4-信号量.md)
