## ④ tasklet

**tasklet** 是 built on **softirq** 的 **动态下半部** — 对 **普通设备驱动** 最友好：**动态创建**、**同 tasklet 绝不并发**，锁模型简单。

| 属性 | 说明 |
|------|------|
| **实现** | 跑在 **`HI_SOFTIRQ`** 或 **`TASKLET_SOFTIRQ`** 上 |
| **生命周期** | **动态** — `tasklet_init()` + `tasklet_schedule()` |
| **并发规则** | **相同 tasklet 绝不会在多 CPU 同时执行** |
| | **不同类型** / **不同实例** 的 tasklet **可以** 并发 |
| **上下文** | 软中断上下文 — **不能睡眠** |

#### 与 softirq 对比

| 对比项 | softirq | tasklet |
|--------|---------|---------|
| 注册 | 静态 `open_softirq` — **子系统级** | 每驱动 **动态** tasklet |
| 同类型多 CPU 并行 | **是** | **否**（同一 tasklet 串行） |
| 锁复杂度 | **高** | **低** |
| 性能上限 | **更高** | 略低（串行化） |
| **驱动默认选择** | 否 — 除非极热路径 | **是** |

#### 基本 API

```c
/* 初始化 — 通常在 probe 里一次 */
void tasklet_init(struct tasklet_struct *t,
                  void (*func)(unsigned long), unsigned long data);

/* 调度 — 常在上半部 ISR 里 */
tasklet_schedule(&dev->t);

/* 禁用/等待（卸载前） */
tasklet_disable(&dev->t);
tasklet_kill(&dev->t);    /* 等待已 schedule 的跑完 */
```

#### 执行语义

```
ISR: tasklet_schedule(&dev->t)
        │
        ▼
标记 tasklet pending ──► TASKLET_SOFTIRQ raised
        │
        ▼
某 CPU 上 irq_exit / ksoftirqd 跑 tasklet_fn
        │
        ▼
dev->t.func(data)  ──► 仅一次（除非再次 schedule）
```

| 语义 | 说明 |
|------|------|
| **同一 tasklet 多次 schedule** | 通常 **合并** — 最多 pending 一次 |
| **tasklet_kill** | 卸载前 **必须** — 保证不再运行 |
| **与 ISR 共享数据** | 用 `spin_lock_irqsave` — ISR 与 tasklet 可能抢锁 |

#### 典型驱动模式

```c
static void my_tasklet_fn(unsigned long data)
{
    struct my_dev *dev = (struct my_dev *)data;
    /* 处理 kfifo 里积累的数据 · 唤醒 waitqueue */
    wake_up_interruptible(&dev->waitq);
}

static irqreturn_t my_isr(int irq, void *dev_id)
{
    struct my_dev *dev = dev_id;
    my_hw_ack(dev);
    my_read_fifo_to_kfifo(dev);
    tasklet_schedule(&dev->t);
    return IRQ_HANDLED;
}
```

#### 何时不用 tasklet

| 场景 | 改用 |
|------|------|
| 需要 **`mutex_lock`** | **workqueue** |
| 需要 **同类型多 CPU 并行**（如自研协议栈） | **softirq** 或 **多 tasklet 实例** |
| 工作 **非常长** | workqueue — 避免占满 softirq 预算 |

**HFT：** 自定义 FPGA/行情卡驱动 **defer** 解析用 tasklet 足够；**不要** 在 tasklet 里做重计算 — 与 **NET_RX softirq** 抢同一 CPU 的 **`%soft`** 时间片。

→ [Ch 8.5](section-8.5-工作队列.md) 可睡眠下半部 · [Ch 8.8](section-8.8-锁定与禁用下半部.md) `spin_lock_bh` · [Ch 6](../../chapter-06-kernel-data-structures/) kfifo

### 常见陷阱

1. 在 6.x 内核中还在用 tasklet——tasklet 已 deprecated，推荐 workqueue/threaded IRQ
2. 混淆 tasklet 和 softirq——tasklet 基于 softirq（TASKLET_SOFTIRQ/HI_SOFTIRQ），是 softirq 的封装
3. 以为同类型 tasklet 可以多 CPU 并发——不行，同类型 tasklet 全局串行化

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** tasklet 的核心特征和限制？

<details><summary>答案</summary>

特征：① 基于 softirq（动态注册，不像 softirq 需编译时注册）。② 同类型 tasklet 不会在多 CPU 上并发执行（全局串行化）。③ 在 softirq 上下文运行，不能睡眠。限制：① 串行化导致性能差（不能利用多核）。② 不能睡眠/持 mutex。③ 已 deprecated。替代方案：workqueue（可并发可睡眠）或 threaded IRQ。

</details>

**Q2.** 为什么 tasklet 要被废弃？用什么替代？

<details><summary>答案</summary>

① 同类型串行化：多核系统上性能瓶颈。② 不能睡眠：限制了使用场景。③ API 复杂且容易误用。替代：需要并发 → workqueue（alloc_workqueue + queue_work）。需要低延迟 + 可睡眠 → threaded IRQ（request_threaded_irq）。需要定时 → hrtimer。内核社区计划在 future version 移除 tasklet API。

</details>

**Q3.** 如果遇到旧代码中的 tasklet，怎么迁移到 workqueue？

<details><summary>答案</summary>

```c
// 旧: tasklet
DECLARE_TASKLET(my_tasklet, my_func, data);
tasklet_schedule(&my_tasklet);
// 新: workqueue
struct work_struct my_work;
INIT_WORK(&my_work, my_func);
schedule_work(&my_work);
// 区别: work 可以睡眠/持mutex, 可多CPU并发
```

</details>

</details>


> ↔ [ULK Ch4 §7 可延迟函数与工作队列](../../../16-linux-kernel-deep/chapter-04-interrupts-and-exceptions/notes/section-7-可延迟函数与工作队列.md)
---
