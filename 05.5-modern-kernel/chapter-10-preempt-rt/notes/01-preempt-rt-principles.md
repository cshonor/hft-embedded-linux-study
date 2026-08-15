# PREEMPT_RT 核心原理：可睡眠自旋锁与中断线程化

> 来源: Bootlin Real-Time Training
> 对标旧书: ULK3 无覆盖 / LKD3 简略

---

## PREEMPT_RT 是什么

PREEMPT_RT (Real-Time patch) 是 Linux 内核的实时补丁，目标：**提供确定性的最大调度延迟**（通常 < 100μs）。

### 延迟保证

| 配置 | 典型最大延迟 | 最坏情况 |
|------|-------------|---------|
| 普通内核 (PREEMPT_NONE) | 1-5 ms | 50+ ms |
| PREEMPT_VOLUNTARY | 100-500 μs | 10+ ms |
| PREEMPT_FULL | 100-500 μs | 5+ ms |
| **PREEMPT_RT** | **50-100 μs** | **< 200 μs** |

```bash
# 测量最大调度延迟
sudo cyclictest -t 1 -p 80 -i 1000 -a 2 -n -d 0 -h 400
# 普通内核: max latency ~500-5000 μs
# PREEMPT_RT: max latency ~50-100 μs (树莓派 5)
```

---

## 核心改动

| 改动 | 普通内核 | PREEMPT_RT | 原因 |
|------|---------|------------|------|
| **自旋锁** | 禁用抢占 | 转为 rt_mutex（可睡眠） | 消除不可抢占区段 |
| **硬中断** | hardirq 上下文 | 线程化为 RT 线程 | 中断可被抢占 |
| **线程化中断** | 可选 | 强制所有中断线程化 | 确保中断不干扰 RT 线程 |
| **抢占模型** | Voluntary (默认) | Full preemption | 任意位置可抢占 |
| **高精度定时器** | 可选 | 强制启用 | 精确时间控制 |
| **rcu_read_lock** | 禁用抢占 | 不禁用抢占 | RCU 临界区可被抢占 |

---

## 可睡眠自旋锁 (rt_mutex)

### 普通内核: spinlock 禁用抢占

```c
// 普通内核中 spin_lock 禁用抢占
spin_lock(&my_lock);
// 抢占被禁用! 这段代码不可被抢占
// 即使有更高优先级的 RT 线程就绪, 也必须等 spin_unlock
spin_unlock(&my_lock);
// 抢占重新启用
```

**问题:** 如果 spin_lock 保护的临界区很长（或被中断打断），高优先级线程必须等待，产生**优先级反转**。

### PREEMPT_RT: spinlock → rt_mutex

```c
// PREEMPT_RT 中 spin_lock 实际使用 rt_mutex
// rt_mutex 持有时可以被抢占/睡眠!
spin_lock(&my_lock);
// 抢占未被禁用! 高优先级 RT 线程可以抢占我
// 如果我被换出, rt_mutex 保证我尽快回来释放锁
spin_unlock(&my_lock);

// rt_mutex 的优先级继承:
// 如果线程 A 持有锁, 线程 B (更高优先级) 等待锁
// → A 临时继承 B 的优先级, 被快速调度执行, 释放锁
// → B 获得锁, 高优先级运行
```

### 优先级继承

```
场景: 优先级反转
  CPU 上: 线程 C (低优先级, 持有锁)
          线程 B (中优先级, 运行中, 阻止 C 执行)
          线程 A (高优先级, 等待 C 的锁)

没有优先级继承:
  B 运行 → C 无法执行 → A 等锁 → A 被中优先级 B "阻塞"

有优先级继承 (rt_mutex):
  A 等锁 → C 继承 A 的优先级 (高于 B)
  → C 抢占 B 执行 → C 释放锁 → A 获得锁, 运行
```

---

## 中断线程化

### 普通内核: 中断不可抢占

```
网卡中断到达 → CPU 跳到中断处理函数 (hardirq)
  → 中断处理执行完毕前, 同级或低级中断被屏蔽
  → 即使有高优先级 RT 线程就绪, 也要等中断处理完成
  → 如果中断处理耗时 200μs, RT 线程延迟 200μs
```

### PREEMPT_RT: 中断变线程

```
网卡中断到达 → 唤醒 irq/32-eth0 线程 (SCHED_FIFO, 优先级 50)
  → 中断线程与普通线程一样参与调度
  → 如果交易线程优先级 80 > 中断线程 50
  → 交易线程可以抢占中断线程!

# PREEMPT_RT 下所有中断变为线程
$ ps -eo pid,comm,policy,rtprio | grep irq/
  42 irq/29-brcmv7   FIFO   50
  43 irq/31-mmc1     FIFO   50
  44 irq/32-eth0     FIFO   50

# 调整中断线程优先级
chrt -f -p 40 44    # 降低网卡中断优先级
# 交易线程 SCHED_FIFO 80 > 网卡中断 50 → 交易线程可抢占网卡中断
```

### 强制线程化 vs 可选线程化

| 特性 | 普通内核 (threadirqs) | PREEMPT_RT |
|------|---------------------|------------|
| 中断线程化 | 可选 (bootargs: threadirqs) | 强制 |
| 线程优先级 | 默认 50 | 默认 50, 可调整 |
| 非线程化中断 | 大部分 hardirq | 仅极少数 (timer, IPI) |
| 延迟确定性 | 中等 | 最高 |

---

## 抢占模型对比

```
PREEMPT_NONE:     只有显式 cond_resched() 点可抢占 (服务器)
PREEMPT_VOLUNTARY: 在 cond_resched() 点 + 某些位置可抢占 (桌面)
PREEMPT_FULL:     除 spin_lock 临界区外都可抢占 (嵌入式)
PREEMPT_RT:       几乎任意位置都可抢占 (实时系统)
                  spin_lock 变为 rt_mutex, 中断变线程
```

| 模型 | 配置 | 延迟 | 适用 |
|------|------|------|------|
| PREEMPT_NONE | 无抢占 | 高 | 服务器（吞吐优先） |
| PREEMPT_VOLUNTARY | 自愿抢占 | 中 | 桌面 |
| PREEMPT_FULL | 完全抢占 | 低 | 嵌入式 |
| PREEMPT_RT | RT 补丁 | 最低（确定性） | 实时系统 |

---

## 与旧书差异

| ULK3 / LKD3 | PREEMPT_RT |
|-------------|------------|
| 无 PREEMPT_RT | RT 补丁是实时系统核心 |
| spinlock 禁用抢占 | RT 中 spinlock 可睡眠 (rt_mutex) |
| 中断不可抢占 | RT 中中断线程化，可被高优先级线程抢占 |
| 无延迟保证 | RT 保证 < 100μs 最大调度延迟 |
| 无优先级继承 | rt_mutex 有优先级继承 |

---

## HFT 关联

| PREEMPT_RT 特性 | HFT 价值 |
|----------------|---------|
| 确定性延迟 | 交易线程的调度延迟可预测 |
| 中断线程化 | 交易线程可抢占网卡中断 |
| 优先级继承 | 避免优先级反转导致延迟 |
| 可睡眠锁 | 不会因为辅助线程持锁而阻塞交易线程 |

> **HFT 实盘：** PREEMPT_RT + SCHED_FIFO 80 + isolcpus + nohz_full = HFT 交易线程的最佳调度配置。交易线程可以抢占几乎所有其他线程和中断，获得确定性的微秒级延迟。

---

## 自测题

<details>
<summary>Q1: PREEMPT_RT 中 spinlock 为什么可以睡眠？不会死锁吗？</summary>

RT 将 spinlock 转为 rt_mutex（实时互斥锁）。持有 rt_mutex 的线程可以被抢占/睡眠，等待者不会自旋而是睡眠排队。不会死锁，因为 rt_mutex 有优先级继承——等待者将自己的优先级传给持有者，确保持有者尽快释放锁。代价是 spinlock 的开销增大（从原子操作变为完整的锁实现）。
</details>

<details>
<summary>Q2: 什么是优先级反转？rt_mutex 如何解决？</summary>

优先级反转: 低优先级线程持有锁，高优先级线程等待锁，中间优先级线程抢占了低优先级线程的 CPU 时间，导致高优先级线程间接被中优先级线程"阻塞"。rt_mutex 通过优先级继承解决: 等待者将优先级传给持有者，持有者临时以高优先级运行，快速释放锁。释放后恢复原优先级。
</details>

<details>
<summary>Q3: PREEMPT_RT 为什么要强制所有中断线程化？</summary>

普通内核中中断在 hardirq 上下文执行，不可被抢占。如果网卡中断处理耗时 200μs，即使有更高优先级的 RT 线程就绪也必须等待。PREEMPT_RT 将中断变为线程（SCHED_FIFO, 优先级 50），如果 RT 线程优先级 > 50，可以抢占中断线程。极少数中断（如 timer, IPI）不线程化，因为它们是调度器本身需要的。
</details>

<details>
<summary>Q4: PREEMPT_RT 的延迟保证是"最大 100μs"还是"平均 100μs"？</summary>

是**最大**延迟（worst-case）。cyclictest 测量的就是最坏情况。PREEMPT_RT 的设计目标是确定性的最坏情况延迟 < 100-200μs，而不是平均值。普通内核平均延迟可能也很好，但最坏情况可能达到 50ms（中断风暴、大量不可抢占区段）。HFT 需要的是确定性（最坏情况可控），不是平均值。
</details>

---

## 交叉引用

- [02-preempt-rt-hft-tuning.md](./02-preempt-rt-hft-tuning.md) — RT 调优参数与 HFT 实践
- [chapter-02-scheduler](../../chapter-02-scheduler/) — EEVDF 调度器
- [chapter-04-synchronization](../../chapter-04-synchronization/) — qspinlock 与 rt_mutex
