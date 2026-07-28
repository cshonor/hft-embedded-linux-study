## ⑤ 工作队列 · Work Queues

**workqueue（工作队列）** 把延迟工作提交给 **内核 worker 线程** 执行 — 处于 **进程上下文**，是 **唯一允许阻塞/睡眠** 的下半部机制。

| 属性 | 说明 |
|------|------|
| **执行者** | **`worker` 内核线程** — 如 `events/n`（每 CPU 一个 pool） |
| **上下文** | **进程上下文** — 有 meaningful `current` |
| **能力** | **`mutex_lock`**、**`kmalloc(GFP_KERNEL)`**、**块 I/O**、等信号量 |
| **代价** | **调度延迟** 高于 tasklet/softirq — 不适合极热路径 |

#### 与其他下半部对比

| 机制 | 上下文 | 睡眠 | 延迟 | 典型 |
|------|--------|------|------|------|
| softirq | 软中断 | 否 | **最低** | NET_RX |
| tasklet | 软中断 | 否 | 低 | 驱动 defer |
| **workqueue** | **进程** | **是** | 较高 | 慢路径、I/O |

```
ISR（hardirq）
    │ schedule_work()
    ▼
work 结构入队 ──► events/N worker 线程被唤醒
                        │
                        ▼
                  work->func() 在进程上下文跑
                        │
                        ├── mutex_lock OK
                        └── 可能 schedule 出去
```

#### 基本 API（LKD 3rd 风格）

```c
#include <linux/workqueue.h>

/* 初始化 */
INIT_WORK(&dev->work, my_work_func);

/* 提交 — 若已在队列则不再重复（同 work 合并） */
schedule_work(&dev->work);

/* 或指定队列 */
queue_work(system_wq, &dev->work);

/* 刷新 / 取消（卸载前） */
flush_work(&dev->work);
cancel_work_sync(&dev->work);
```

```c
static void my_work_func(struct work_struct *work)
{
    struct my_dev *dev = container_of(work, struct my_dev, work);
    /* 可睡眠：读 I2C · 分配大块内存 · 文件 I/O */
    mutex_lock(&dev->lock);
    /* … */
    mutex_unlock(&dev->lock);
}
```

#### 默认队列 vs 自建

| 队列 | 说明 |
|------|------|
| **`system_wq` / `events/n`** | 每 CPU **通用** worker — **大多数驱动够用** |
| **`system_long_wq`** | 可能跑 **较长** 任务 |
| **`create_singlethread_workqueue()`** | 驱动私有 **单线程** 队列 — 保证顺序 |
| **`alloc_ordered_workqueue()`** | 严格 FIFO 顺序（现代 API 演进） |

| 选择 | 场景 |
|------|------|
| **`schedule_work`** | 简单 defer、无顺序要求 |
| **专用 workqueue** | 避免 **堵塞全局 events 池**、要 **固定顺序** |

#### 典型使用场景

| 场景 | 为何用 workqueue |
|------|------------------|
| **I2C/SPI 寄存器访问** | 可能睡眠 |
| **udev 热插拔后续处理** | 复杂、可阻塞 |
| **`GFP_KERNEL` 大块分配** | 可能触发 reclaim |
| **filesystem 写日志** | 可能等磁盘 |

#### 与 ISR 同步

| 问题 | 解法 |
|------|------|
| ISR 与 work **共享数据** | `spinlock` 护短临界区；work 里用 `mutex` 护长段 |
| **卸载时 work 仍 pending** | `cancel_work_sync()` **必须** 在 `free_irq` 前后正确排序 |
| **重复 schedule** | 同一 `work_struct` 并发 submit 需 **`queue_work` 返回值** 或 flush |

#### 常见模式：中断 + kfifo + workqueue

| 阶段 | 做什么 |
|------|--------|
| **ISR** | 数据入 **kfifo**（Ch 6）— `spin_lock_irqsave` |
| **tasklet 或 schedule_work** | 通知下半部 |
| **work fn** | 从 kfifo 取出、**阻塞式** 处理、唤醒用户态 |

**HFT：** 主收包路径 **不应** 用 workqueue — 延迟不可控。workqueue 适合 **慢路径**：写 pcap 到磁盘、重配置 NIC、非实时统计。

→ **Ch 6** `kfifo` 中断入队 + workqueue 出队 · [Ch 7.5](../../chapter-07-interrupts/notes/section-7.5-中断上下文.md) 为何 ISR 不能睡眠 · [Ch 8.7](section-8.7-如何选择下半部机制.md) 选型

---
