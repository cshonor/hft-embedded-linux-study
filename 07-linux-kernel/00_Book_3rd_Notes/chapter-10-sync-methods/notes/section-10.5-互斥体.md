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

---
