# Bootlin: 中断与并发同步

> **来源:** [Bootlin Kernel Training](https://bootlin.com/docs/kernel/)
> **主题:** 中断处理 + 内核同步机制
> **对标旧书:** ULK3 Ch4+Ch5 / LKD3 Ch7+Ch9

---

## 讲义要点

### 中断处理层次 (6.x)

```
硬件中断 → GIC → IRQ Domain 映射 → hardirq handler
                                        ↓
                                   (返回 IRQ_WAKE_THREAD?)
                                        ↓
                              threaded IRQ handler (可睡眠)
                    OR
                    softirq / tasklet (6.x 废弃中) / workqueue
```

### 6.x 中断处理变化

| 机制 | 2.6 时代 | 6.x 现代 |
|------|---------|---------|
| 下半部 | tasklet / softirq | threaded IRQ (推荐) / workqueue |
| tasklet | 广泛使用 | 废弃中，用 threaded IRQ 替代 |
| softirq | 9 种 | 10 种 (新增 RCU softirq) |
| workqueue | 2 种类型 | 统一为 WQ (workqueue) + pwq |

### 同步机制选型 (6.x)

| 机制 | 能否睡眠 | 开销 | 适用场景 |
|------|---------|------|---------|
| **spinlock** | ❌ | 极低 | 短临界区，中断上下文 |
| **qspinlock** | ❌ | 极低（高争用更优） | 6.x 默认自旋锁 |
| **mutex** | ✅ | 中 | 长临界区，进程上下文 |
| **rwlock** | ❌ | 低 | 读多写少（但 RCU 更好） |
| **rwsem** | ✅ | 中 | 读多写少，可睡眠 |
| **RCU** | 读端❌/写端✅ | 读端零开销 | 读多写少，指针替换 |
| **SRCU** | ✅ | 读端低开销 | 读多写少，需睡眠 |
| **seqlock** | ❌ | 极低 | 读多写少，写者罕见 |
| **percpu** | N/A | 极低 | per-CPU 计数器/数据 |

### 关键规则

1. **中断上下文**：只能用 spinlock / RCU / percpu，不能用 mutex / rwsem
2. **进程上下文 + 需要睡眠**：用 mutex / rwsem
3. **读多写少 + 指针替换**：用 RCU（读端零开销）
4. **per-CPU 数据**：用 `local_irq_disable()` + percpu 变量

### PREEMPT_RT 的影响

| 机制 | 普通内核 | PREEMPT_RT |
|------|---------|------------|
| spinlock | 禁用抢占 | 转为 rt_mutex（可睡眠） |
| 硬中断 | hardirq | 线程化为 RT 线程 |
| 禁用抢占段 | 不可抢占 | 可被高优先级 RT 线程抢占 |

---

## 动手实验

```bash
# 1. 查看中断统计
cat /proc/interrupts
watch -n 1 cat /proc/interrupts   # 实时监控

# 2. 查看中断绑核
cat /proc/irq/<irq>/smp_affinity  # 十六进制 CPU mask
echo 04 > /proc/irq/29/smp_affinity  # 绑定到 CPU 2

# 3. 查看 softirq 统计
cat /proc/softirqs

# 4. 查看中断线程 (threaded IRQ / PREEMPT_RT)
ps -eo pid,comm,policy,rtprio | grep "irq/"

# 5. 查看锁竞争 (lockdep)
echo 1 > /proc/sys/kernel/lock_stat  # 启用锁统计
cat /proc/lock_stat                    # 查看锁竞争

# 6. 查看抢占模型
cat /sys/kernel/debug/sched/preempt
# 或
zcat /proc/config.gz | grep PREEMPT
```

---

## 与旧书差异

| ULK3 讲的 | Bootlin 讲义 |
|-----------|-------------|
| tasklet 是下半部主力 | tasklet 废弃中，用 threaded IRQ |
| ticket spinlock | qspinlock (MCS 队列) |
| 大内核锁 (BKL) | 已删除 (2.6.37) |
| 基础 RCU | Tree RCU + SRCU + lazy RCU |
| 中断不能被抢占 | PREEMPT_RT 下中断可被 RT 线程抢占 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么在持有 spinlock 时不能睡眠？

> 普通内核中 spinlock 禁用抢占。如果睡眠，调度器会切换到其他线程，但其他线程如果也尝试获取同一 spinlock 会自旋等待——由于持有者被调度走，锁永远无法释放，导致死锁。PREEMPT_RT 中 spinlock 转为 rt_mutex 可以睡眠，但行为不同。

**Q2:** 6.x 为什么废弃 tasklet？

> tasklet 在 softirq 上下文执行，不能睡眠，且有两个 tasklet 类型（HI 和 normal）增加复杂性。threaded IRQ 提供了更好的替代——在内核线程中执行，可以睡眠、可以持互斥锁、可以被高优先级线程抢占（PREEMPT_RT）。

**Q3:** RCU 读端为什么 "零开销"？

> `rcu_read_lock()` 只是禁用抢占（或禁用迁移），不获取任何锁。reader 通过 `rcu_dereference()` 读取指针（编译器屏障），然后使用数据。没有原子操作、没有缓存行弹跳、没有锁争用。开销仅是禁用抢占的指令。

</details>
