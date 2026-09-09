# 05 · 中断与并发：硬中断里为什么不能 sleep

> **本节讲什么：** 中断上下文的约束、上下半部切分、以及内核里"锁"的正确选型。
> **这是最容易写崩的一块** —— 违规不会立刻报错，而是随机死锁或踩内存。

---

## 铁律

| 规则 | 原因 |
|------|------|
| **硬中断上下文不能睡眠** | 中断不关联任何进程，没有可被调度的上下文；睡了就再也醒不过来 |
| **不能调用可能睡眠的函数** | `kmalloc(GFP_KERNEL)`、`mutex_lock`、`copy_to_user`、`msleep` 全禁 |
| **中断处理要快** | 关中断期间整个 CPU 不响应其它中断，直接恶化延迟 |
| **持自旋锁时不能睡眠** | 别人在忙等你，你睡了 → 死锁（抢占内核下还会拖死整个核） |

---

## 上半部 / 下半部

```
硬中断 top half      →  只做最紧急的：清中断、读数、调度下半部
       ↓
下半部 bottom half   →  可以慢一点做的工作
```

| 下半部机制 | 上下文 | 能睡？ |
|-----------|--------|--------|
| **softirq** | 中断（软中断） | ❌ |
| **tasklet** | 基于 softirq | ❌ |
| **workqueue** | 内核线程 | ✅ |
| **threaded IRQ**（现代推荐） | 内核线程 | ✅ |

**现代写法：** `request_threaded_irq()` —— handler 跑在硬中断（快速判断/清中断），thread_fn 跑在内核线程（可以睡、可以调 I2C/SPI）。比手写 tasklet/workqueue 简单且安全。

---

## 锁怎么选

| 场景 | 用什么 |
|------|--------|
| 临界区很短、可能在中断上下文 | `spinlock_t`（+ 需要时 `spin_lock_irqsave`） |
| 临界区可以睡（会调 I2C/SPI/拷贝用户数据） | `mutex` |
| 读多写少 | `rwlock` / `seqlock` / RCU |
| per-CPU 数据 | `local_irq_disable` 或 per-cpu 变量（**最快的锁是不加锁**） |

**顺序原则：** 永远按固定顺序获取多把锁；中断 handler 里必须用 `spin_lock_irqsave`（否则本核中断再来一次就自锁死）。

---

## 关键代码

```c
static irqreturn_t my_handler(int irq, void *dev_id)
{
    /* 硬中断：清中断源，尽快返回 */
    return IRQ_WAKE_THREAD;
}

static irqreturn_t my_thread(int irq, void *dev_id)
{
    mutex_lock(&dev->lock);        /* 线程上下文，可以睡 */
    /* 读 I2C 寄存器、拷贝数据 */
    mutex_unlock(&dev->lock);
    return IRQ_HANDLED;
}

request_threaded_irq(irq, my_handler, my_thread,
                     IRQF_TRIGGER_FALLING, "mydev", dev);
```

---

## HFT / 嵌入式关联

- **中断处理时间 = 延迟抖动源**。这是 HFT 里"为什么要把中断绑到隔离核、为什么要 NAPI"的根因：NAPI 本质是 **中断 + 轮询的混合**（收一个中断后进轮询，避免每包一中断），见 [12.5-modern-networking](../../12.5-modern-networking)。
- **per-CPU 数据结构避免加锁**，这个思路在 HFT 用户态无锁队列里是同一个招式（[14-hft-engineering](../../14-hft-engineering)）。
- 锁的选型本质是"能不能睡"，而"能不能睡"取决于**当前在什么上下文**——判断上下文是内核编程的基本功。

---

## 本节笔记

| # | 笔记 |
|---|------|
| 5.1 | [中断上下文：为什么不能 sleep](./5.1-irq-context.md) |
| 5.2 | [锁的选型：只看"能不能睡"](./5.2-lock-selection.md) |
| 5.3 | [threaded IRQ 实践](./5.3-threaded-irq.md) |

---

## 验收

- [ ] 能说出至少 3 个在硬中断里**不能**调用的函数
- [ ] 知道 `IRQ_WAKE_THREAD` + `request_threaded_irq` 的用法与好处
- [ ] 能解释为什么持自旋锁睡眠会死锁
- [ ] 能按"能否睡眠"正确选择 spinlock / mutex

---

## 衔接

- **上一步：** [04-gpio-i2c-spi](../04-gpio-i2c-spi/)
- **下一步：** [06-dma-mmap](../06-dma-mmap/)
- **卡住查书：** Madieu Ch16、Ch3 · LDD3 Ch5、Ch10、Ch7
