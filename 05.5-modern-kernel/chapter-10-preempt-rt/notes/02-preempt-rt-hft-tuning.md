# PREEMPT_RT 调优与 HFT 实践

> 来源: Bootlin Real-Time Training
> 前置: [01-preempt-rt-principles.md](./01-preempt-rt-principles.md)

---

## HFT RT 调优清单

### 1. 内核编译选项

```bash
# .config 中必须开启的选项
CONFIG_PREEMPT_RT=y           # 实时抢占
CONFIG_HIGH_RES_TIMERS=y      # 高精度定时器 (RT 强制)
CONFIG_HZ_1000=y              # 1000 Hz 时钟
CONFIG_NO_HZ_FULL=y           # 无滴带支持
CONFIG_CPU_ISOLATION=y        # CPU 隔离

# 开发期 (生产关闭)
CONFIG_KASAN=n                # RT 下 KASAN 不可用
CONFIG_KFENCE=y               # 轻量内存检测
CONFIG_DEBUG_INFO=y           # 调试符号
```

### 2. 内核启动参数

```bash
# /boot/cmdline.txt (树莓派) 或 GRUB
isolcpus=2-3              # 隔离 CPU 2-3
nohz_full=2-3             # CPU 2-3 进入无滴带模式
rcu_nocbs=2-3             # RCU 回调卸载到其他核
irqaffinity=0-1           # IRQ 默认绑到 CPU 0-1
threadirqs                # 强制中断线程化 (RT 已默认)
```

### 3. 运行时配置

```bash
# === CPU 频率 ===
echo performance > /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# === IRQ 绑定 ===
systemctl stop irqbalance
# 网卡中断绑到 CPU 0-1 (非隔离核)
echo 03 > /proc/irq/32/smp_affinity   # 二进制 11 = CPU 0+1

# === 中断线程优先级 ===
chrt -f -p 40 $(pgrep irq/32-eth0)    # 网卡中断降到 40

# === 交易线程 ===
chrt -f 80 ./trading_engine           # SCHED_FIFO 优先级 80
```

---

## CPU 隔离详解

### isolcpus + nohz_full + rcu_nocbs 三件套

| 参数 | 作用 | 效果 |
|------|------|------|
| `isolcpus=2-3` | CPU 2-3 不参与负载均衡 | 普通进程不会调度到 CPU 2-3 |
| `nohz_full=2-3` | CPU 2-3 进入无滴带模式 | 减少定时器中断 (从 1000Hz 降到接近 0) |
| `rcu_nocbs=2-3` | RCU 回调卸载 | RCU grace period 的回调不在 CPU 2-3 执行 |

### 验证隔离效果

```bash
# 1. 确认 CPU 隔离
cat /sys/devices/system/cpu/isolated
# 2-3

# 2. 确认 nohz_full
cat /sys/devices/system/cpu/nohz_full
# 2-3

# 3. 确认 RCU 卸载
cat /sys/devices/system/cpu/rcu_nocbs
# 2-3

# 4. 测量隔离核上的中断频率
perf stat -C 2 -e irq:irq_handler_entry sleep 5
# 如果 nohz_full 生效, 中断次数应该极少

# 5. 检查定时器
cat /proc/timer_list | grep "CPU 2"
# hrtimer 数量应该极少
```

---

## cyclictest 延迟测试

```bash
# 安装 RT 测试工具
sudo apt install rt-tests

# === 基准延迟测试 ===
sudo cyclictest -t 1 -p 80 -i 1000 -l 1000000 -a 2 -n
# -t 1: 1 线程
# -p 80: SCHED_FIFO 优先级 80
# -i 1000: 1ms 间隔
# -l 1000000: 100 万次
# -a 2: 绑定 CPU 2 (隔离核)
# -n: 使用 clock_nanosleep

# 典型输出:
# T: 1 ( 1234) P:80 I:1000 C: 1000000 Min: 12 Act: 15 Avg: 18 Max: 87
# Max = 87μs → 在 100μs 以内, 合格

# === 压力下延迟测试 ===
# 终端 1: cyclictest
sudo cyclictest -t 1 -p 80 -i 1000 -a 2 -n &
# 终端 2: CPU 压力
stress-ng --cpu 4 --io 2 --vm 2 --vm-bytes 256M --timeout 60s &
# 终端 3: I/O 压力
stress-ng --hdd 2 --timeout 60s &
# 观察 cyclictest 的 Max 值变化

# === hackbench 压力 ===
hackbench -l 10000 &
sudo cyclictest -t 1 -p 80 -i 1000 -a 2 -n
# Max 应该仍然 < 200μs (PREEMPT_RT 保证)
```

### 延迟分布直方图

```bash
# 生成延迟直方图
sudo cyclictest -t 1 -p 80 -i 1000 -a 2 -n -h 400 > latency_hist.txt
# -h 400: 直方图, 0-400μs
# 查看分布:
#  00-09μs: ##########
#  10-19μs: ####################
#  20-29μs: ########
#  30-39μs: ##
#  40-49μs: #
#  50+μs:   (应该极少)
```

---

## 交易线程完整配置脚本

```bash
#!/bin/bash
# hft_setup.sh — HFT 交易线程运行环境配置

# 1. CPU 频率
for cpu in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
    echo performance > "$cpu"
done

# 2. 禁用 irqbalance
systemctl stop irqbalance 2>/dev/null

# 3. 网卡中断绑到 CPU 0-1
for irq in $(grep -l eth0 /proc/irq/*/name 2>/dev/null | sed 's|/proc/irq/||;s|/name||'); do
    echo 03 > "/proc/irq/$irq/smp_affinity"
    chrt -f -p 40 "$(pgrep "irq/$irq" 2>/dev/null)" 2>/dev/null
done

# 4. 锁定内存页
# (在程序中调用 mlockall(MCL_CURRENT | MCL_FUTURE))

# 5. 启动交易引擎
chrt -f 80 taskset -c 2 ./trading_engine
```

```c
// 交易引擎中的 RT 配置
#include <sched.h>
#include <sys/mman.h>

void setup_rt(void) {
    // 1. 锁定内存 (防止 page fault)
    mlockall(MCL_CURRENT | MCL_FUTURE);

    // 2. 设置 SCHED_FIFO
    struct sched_param param = { .sched_priority = 80 };
    sched_setscheduler(0, SCHED_FIFO, &param);

    // 3. 绑定 CPU 2
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(2, &cpuset);
    sched_setaffinity(0, sizeof(cpuset), &cpuset);
}
```

---

## 常见延迟问题排查

| 症状 | 可能原因 | 排查工具 |
|------|---------|---------|
| Max latency > 200μs | SMI 干扰 | `hwlat` tracer |
| 偶发性尖峰 | 页错误 | `perf stat -e page-faults` |
| 持续高延迟 | CPU 频率缩放 | `cpufreq` governor 检查 |
| 中断相关延迟 | IRQ 绑定错误 | `cat /proc/interrupts` |
| 调度延迟 | 优先级设置错误 | `chrt -p <pid>` 确认 |
| RCU 回调干扰 | rcu_nocbs 未设置 | `grep rcu /sys/devices/system/cpu/` |

```bash
# 使用 ftrace 诊断延迟
echo wakeup_rt > /sys/kernel/tracing/current_tracer
echo 1 > /sys/kernel/tracing/tracing_on
# ... 运行交易引擎 ...
cat /sys/kernel/tracing/tracing_max_latency
# 如果 > 100μs, 查看 trace 看延迟发生在哪里

echo irqsoff > /sys/kernel/tracing/current_tracer
cat /sys/kernel/tracing/tracing_max_latency
# 如果 > 100μs, 有关中断被关闭太久
```

---

## HFT 关联

| 调优项 | HFT 价值 | 不做的后果 |
|--------|---------|-----------|
| SCHED_FIFO 80 | 交易线程最高优先级 | 被其他线程抢占 |
| isolcpus | 独占 CPU | 其他进程干扰 |
| nohz_full | 减少定时器中断 | 1000Hz 中断干扰 |
| mlockall | 防止 page fault | 偶发 100μs+ 延迟 |
| IRQ 绑定 | 网卡中断不干扰交易核 | 中断打断交易线程 |
| performance governor | CPU 频率不变 | 频率切换导致延迟变化 |

> **HFT 黄金法则：** SCHED_FIFO + isolcpus + nohz_full + mlockall + performance governor。五者缺一不可。PREEMPT_RT 是底座，提供可抢占的内核环境。

---

## 自测题

<details>
<summary>Q1: isolcpus=2 nohz_full=2 rcu_nocbs=2 三个参数各自的作用？</summary>

`isolcpus=2`：CPU 2 不参与负载均衡，不自动调度普通进程。`nohz_full=2`：CPU 2 进入无滴带模式，减少定时器中断（从每秒 1000 次降到接近 0）。`rcu_nocbs=2`：CPU 2 的 RCU 回调卸载到其他核。三者配合将 CPU 2 完全隔离给 RT 线程，最大化确定性。
</details>

<details>
<summary>Q2: 为什么要把网卡中断线程的优先级设低于交易线程？</summary>

PREEMPT_RT 下中断是线程。如果网卡中断优先级（默认 50）高于交易线程（80），网卡中断会抢占交易线程，增加延迟。设交易线程优先级 > 中断线程优先级，交易线程可以抢占中断处理，确保行情处理不被中断干扰。网卡中断在 CPU 0-1 上处理（非隔离核），交易线程在 CPU 2 上运行，两者不直接争用 CPU。
</details>

<details>
<summary>Q3: cyclictest 测量 Max latency = 150μs，正常吗？</summary>

取决于硬件和配置。树莓派 5 上 PREEMPT_RT 的典型 Max latency 在 50-100μs。如果测到 150μs，可能原因：① SMI (System Management Interrupt) 干扰——用 hwlat tracer 确认；② CPU 频率缩放未关闭——检查 scaling_governor；③ nohz_full 未生效——检查 /sys/devices/system/cpu/nohz_full；④ 内存未锁定——确认 cyclictest 用了 mlockall。在 x86 服务器上 150μs 可能是正常的（SMI 开销大）。
</details>

<details>
<summary>Q4: mlockall 为什么对 HFT 至关重要？</summary>

mlockall 锁定进程所有内存页，防止 page fault。page fault 时内核需要从磁盘换入页面，延迟可达 100μs-10ms，完全破坏 RT 保证。即使系统内存充足，内核也可能因为预读、COW（写时复制）等机制触发 page fault。mlockall(MCL_CURRENT | MCL_FUTURE) 锁定当前和未来分配的所有页面，是 HFT 的必备配置。
</details>

---

## 交叉引用

- [01-preempt-rt-principles.md](./01-preempt-rt-principles.md) — PREEMPT_RT 核心原理
- [chapter-12-vdso-debugging](../chapter-12-vdso-debugging/) — ftrace/eBPF 延迟排查
- [chapter-02-scheduler](../chapter-02-scheduler/) — EEVDF 与 SCHED_FIFO
