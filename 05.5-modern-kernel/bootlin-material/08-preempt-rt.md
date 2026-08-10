# Bootlin: PREEMPT_RT 实时内核

> **来源:** [Bootlin Real-Time Training](https://bootlin.com/docs/realtime/)
> **主题:** PREEMPT_RT 实时抢占补丁
> **对标旧书:** ULK3 无覆盖 / LKD3 简略

---

## 讲义要点

### PREEMPT_RT 核心改动

| 改动 | 普通内核 | PREEMPT_RT |
|------|---------|------------|
| **自旋锁** | 禁用抢占 | 转为 rt_mutex（可睡眠） |
| **硬中断** | hardirq 上下文 | 线程化为 RT 线程 |
| **线程化中断** | 可选 | 强制所有中断线程化 |
| **抢占模型** | Voluntary (默认) | Full preemption |
| **高精度定时器** | 可选 | 强制启用 |
| **rcu_read_lock** | 禁用抢占 | 不禁用抢占（改为 rt_mutex 保护） |

### 抢占模型对比

| 模型 | 配置 | 延迟 | 适用 |
|------|------|------|------|
| `PREEMPT_NONE` | 无抢占 | 高 | 服务器（吞吐优先） |
| `PREEMPT_VOLUNTARY` | 自愿抢占 (cond_resched) | 中 | 桌面 |
| `PREEMPT_FULL` | 完全抢占 | 低 | 嵌入式 |
| `PREEMPT_RT` | RT 补丁 | 最低（确定性） | 实时系统 |

### PREEMPT_RT 延迟保证

```bash
# 测量最大调度延迟
sudo cyclictest -t 1 -p 80 -i 1000 -a 2 -n -d 0 -h 400
# 典型结果:
#   普通内核: max latency ~500-5000 μs
#   PREEMPT_RT: max latency ~50-100 μs (树莓派 5)
```

| 配置 | 典型最大延迟 | 最坏情况 |
|------|-------------|---------|
| 普通内核 (non-RT) | 1-5 ms | 50+ ms |
| PREEMPT (non-RT) | 100-500 μs | 10+ ms |
| PREEMPT_RT | 50-100 μs | < 200 μs |

### 中断线程化

```bash
# PREEMPT_RT 下所有中断变为线程
$ ps -eo pid,comm,policy,rtprio | grep irq/
  42 irq/29-brcmv7   FIFO   50
  43 irq/31-mmc1     FIFO   50
  44 irq/32-eth0     FIFO   50

# 调整中断线程优先级
chrt -f -p 40 44    # 降低网卡中断优先级
# 交易线程 SCHED_FIFO 80 > 网卡中断 50 → 交易线程可抢占网卡中断
```

### RT 调优关键参数

```bash
# 1. 隔离 CPU (GRUB 参数)
isolcpus=2-3 nohz_full=2-3 rcu_nocbs=2-3

# 2. 设置 RT 优先级
chrt -f 80 ./trading_app   # FIFO 80

# 3. 锁定内存页
# 在程序中调用 mlockall(MCL_CURRENT | MCL_FUTURE)

# 4. 设置 CPU 调度域 (隔离核不参与负载均衡)
# /sys/devices/system/cpu/cpu2/cpufreq/scaling_governor = performance
# /sys/devices/system/cpu/cpu2/online = 1 (不 offload)

# 5. 禁用 CPU 频率缩放
echo performance > /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# 6. 禁用 IRQ balance
systemctl stop irqbalance
# 手动绑定 IRQ 到非隔离核
echo 03 > /proc/irq/32/smp_affinity  # 网卡中断绑到 CPU 0-1
```

---

## 动手实验

```bash
# 1. 检查 PREEMPT_RT 支持
uname -a
# Linux ... PREEMPT_RT ... # 有 RT 标记
zcat /proc/config.gz | grep PREEMPT_RT
# CONFIG_PREEMPT_RT=y

# 2. 安装 RT 测试工具
sudo apt install rt-tests

# 3. 基准延迟测试
sudo cyclictest -t 1 -p 80 -i 1000 -l 1000000 -a 2 -n
# -t 1: 1 线程
# -p 80: SCHED_FIFO 优先级 80
# -i 1000: 1ms 间隔
# -l 1000000: 100 万次
# -a 2: 绑定 CPU 2
# -n: 使用 clock_nanosleep

# 4. 压力下延迟测试
# 终端 1: 运行 cyclictest
sudo cyclictest -t 1 -p 80 -i 1000 -a 2 -n &
# 终端 2: 压力测试
stress-ng --cpu 4 --io 2 --vm 2 --vm-bytes 256M --timeout 60s &
# 观察 cyclictest 的 max latency

# 5. hackbench 压力下的延迟
hackbench -l 10000 &
cyclictest -t 1 -p 80 -i 1000 -a 2 -n
```

---

## 与旧书差异

| ULK3 / LKD3 | Bootlin 讲义 |
|-------------|-------------|
| 无 PREEMPT_RT | RT 补丁是实时系统核心 |
| spinlock 禁用抢占 | RT 中 spinlock 可睡眠 |
| 中断不可抢占 | RT 中中断线程化，可被高优先级线程抢占 |
| 无延迟保证 | RT 保证 < 100μs 最大调度延迟 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** PREEMPT_RT 中 spinlock 为什么可以睡眠？不会死锁吗？

> RT 将 spinlock 转为 rt_mutex（实时互斥锁）。持有 rt_mutex 的线程可以被抢占/睡眠，等待者不会自旋而是睡眠排队。不会死锁，因为 rt_mutex 有优先级继承——等待者将自己的优先级传给持有者，确保持有者尽快释放锁。代价是 spinlock 的开销增大。

**Q2:** 为什么要将网卡中断线程的优先级设低于交易线程？

> PREEMPT_RT 下中断是线程。如果网卡中断优先级（默认 50）高于交易线程（80），网卡中断会抢占交易线程，增加延迟。设交易线程优先级 > 中断线程优先级，交易线程可以抢占中断处理，确保行情处理不被中断干扰。

**Q3:** `isolcpus=2 nohz_full=2 rcu_nocbs=2` 三个参数各自的作用？

> `isolcpus=2`：CPU 2 不参与负载均衡，不自动调度普通进程。`nohz_full=2`：CPU 2 进入无滴答模式，减少定时器中断（从每秒 100-1000 次降到接近 0）。`rcu_nocbs=2`：CPU 2 的 RCU 回调卸载到其他核。三者配合将 CPU 2 完全隔离给 RT 线程。

</details>
