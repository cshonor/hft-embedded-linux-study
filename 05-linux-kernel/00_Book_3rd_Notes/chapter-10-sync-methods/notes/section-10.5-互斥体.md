## ⑤ 互斥体 · Mutexes

专为 **互斥** 设计的睡眠锁 — **新内核代码里「需要睡眠的互斥」首选**（优于用 semaphore 凑合）。

| 属性 | 说明 |
|------|------|
| 争用 | **睡眠** |
| 上下文 | **仅进程上下文** |
| 持有者 | **有明确 owner**（便于调试、优先级继承等） |

#### 四条严格规则（Love 强调）

| # | 规则 |
|---|------|
| 1 | **只有持有者可以 unlock** |
| 2 | **禁止递归加锁**（同任务二次 lock → 死锁） |
| 3 | **不可在中断/原子上下文使用** |
| 4 | **必须用正式 API 初始化**（勿手搓脏内存当 mutex） |

另：持锁期间 **可以调度/睡眠**（这是相对 spinlock 的意义），但勿在持锁时做无界长时间工作拖垮系统。

#### API 直觉

```c
struct mutex m;
mutex_init(&m);

mutex_lock(&m);
/* 临界区：可 sleep，但尽量短 */
mutex_unlock(&m);

/* 可中断等待 */
if (mutex_lock_interruptible(&m) == 0) {
    /* ... */
    mutex_unlock(&m);
}
```

| API | 用途 |
|-----|------|
| `mutex_lock` | 不可中断睡等 |
| `mutex_lock_interruptible` | 信号可打断 |
| `mutex_trylock` | 不睡 |
| `mutex_is_locked` | 查询（慎用于逻辑） |

#### 选型

| 场景 | 选 |
|------|-----|
| 短、不睡、可在中断 | **spinlock** |
| 长、可睡、仅进程上下文 | **mutex** |
| 计数资源 | **semaphore** |

**HFT：** 用户态 `std::mutex` / `pthread_mutex` 对应层；交易热路径用无锁/原子，**配置重载、会话管理** 用 mutex。内核驱动：`probe`/ioctl 慢路径用 mutex，硬中断里绝不用。

→ [10.2](./section-10.2-自旋锁.md) · [10.4](./section-10.4-信号量.md) · [10.11](./section-10.11-选型速查Ch-9--Ch-10.md)

### 常见陷阱

1. 在 mutex 和 semaphore 之间纠结——内核新代码始终优先 mutex
2. 以为 mutex_lock() 一定睡眠——无争用时直接原子获取（fast path），不进内核
3. 忽略优先级继承——mutex 默认不支持，需要 rt_mutex

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** mutex 的 fast path / slow path 是什么？

<details><summary>答案</summary>

Fast path（无争用）：`mutex_lock()` → 原子 CAS 将 owner 从 NULL 设为 current → 成功返回。开销 ~20ns。Slow path（有争用）：CAS 失败 → `__mutex_lock_slowpath()` → 加入等待队列 → `schedule()` 睡眠 → 被唤醒后重试 CAS。开销 ~1-5us。大部分场景 fast path 命中，mutex 性能接近 spinlock。

</details>

**Q2.** mutex 和 rt_mutex 的区别？HFT 为什么要关心？

<details><summary>答案</summary>

mutex：无优先级继承。高优先级线程等低优先级线程持有的 mutex 时，低优先级线程不会被提升 → 优先级反转 → 高优先级线程延迟增大。rt_mutex：有优先级继承。高优先级等锁时，持有者的优先级被临时提升到等待者的级别。HFT 必须用 rt_mutex（或 `PTHREAD_PRIO_INHERIT`）防止优先级反转。

</details>

**Q3.** HFT 中 mutex 的使用最佳实践？

<details><summary>答案</summary>

① 热路径避免 mutex——用无锁设计。② 必须用 mutex 时：短临界区 + `try_lock` + 超时。③ `PTHREAD_PRIO_INHERIT` 属性防止优先级反转。④ `PTHREAD_MUTEX_ADAPTIVE_NP`：先 spin 再 sleep（glibc 扩展）。⑤ `perf lock` 分析持有时间。⑥ 避免 `std::mutex` 在 RT 线程中使用——用 `std::atomic` 或无锁队列。

</details>

</details>


> ↔ [ULK Ch5 §6 信号量与完成变量](../../../../20-linux-kernel-deep/chapter-05-kernel-synchronization/notes/section-6-信号量与完成变量.md)
---
