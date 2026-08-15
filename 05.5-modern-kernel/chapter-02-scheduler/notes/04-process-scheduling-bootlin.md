# Bootlin: 进程管理与调度

> **来源:** [Bootlin Kernel Training](https://bootlin.com/docs/kernel/)
> **主题:** 进程管理与调度器
> **对标旧书:** ULK3 Ch3+Ch7 / LKD3 Ch3+Ch4

---

## 讲义要点

### 进程 / 线程 / 任务

| 概念 | 内核表示 | 说明 |
|------|---------|------|
| 进程 | `task_struct` + 独立 `mm_struct` | 独立地址空间 |
| 线程 | `task_struct` + 共享 `mm_struct` | 共享地址空间，`clone(CLONE_VM)` 创建 |
| 内核线程 | `task_struct` + `mm = NULL` | 无用户空间地址 |

### 调度类 (6.x)

| 调度类 | 优先级范围 | 说明 |
|--------|-----------|------|
| Stop | 最高 | 内核内部使用（migration 线程） |
| DL (Deadline) | -1 ~ 98 | SCHED_DEADLINE，基于期限的实时调度 |
| RT (Real-time) | 1 ~ 99 | SCHED_FIFO / SCHED_RR |
| CFS/EEVDF | 100 ~ 139 | SCHED_NORMAL / SCHED_BATCH / SCHED_IDLE |
| Idle | 最低 | 空闲调度类 |

### CFS → EEVDF 演进 (6.6+)

| 特性 | CFS (2.6.23 ~ 6.5) | EEVDF (6.6+) |
|------|---------------------|--------------|
| 核心指标 | vruntime | lag + virtual deadline |
| 延迟控制 | 启发式 (wakeup preemption) | latency-nice |
| 时间片 | 基于 sched_latency_ns | 基于 latency-nice + 权重 |
| 一致性 | 一般（启发式导致不可预测） | 更一致（数学框架统一） |

### 调度策略 API

```c
// 设置调度策略
struct sched_param param = { .sched_priority = 80 };
sched_setscheduler(0, SCHED_FIFO, &param);  // RT FIFO, 优先级 80

// 绑核
cpu_set_t cpuset;
CPU_ZERO(&cpuset);
CPU_SET(2, &cpuset);  // 绑定到 CPU 2
sched_setaffinity(0, sizeof(cpuset), &cpuset);

// 查看调度信息
cat /proc/<pid>/sched
chrt -p <pid>         # 查看调度策略和优先级
```

### cgroup v2 CPU 控制

```bash
# 创建 cgroup 限制 CPU
mkdir /sys/fs/cgroup/myapp
echo "100000 1000000" > /sys/fs/cgroup/myapp/cpu.max  # 10% CPU (100ms/1000ms)
echo <pid> > /sys/fs/cgroup/myapp/cgroup.procs
```

---

## 动手实验

```bash
# 1. 查看所有进程的调度策略
ps -eo pid,comm,policy,rtprio,ni,cls --sort=-rtprio | head -20

# 2. 设置进程为 RT FIFO
sudo chrt -f -p 80 <pid>      # SCHED_FIFO 优先级 80

# 3. 测量调度延迟
# 使用 cyclictest (rt-tests 包)
sudo cyclictest -t 1 -p 80 -i 1000 -a 2 -n
# -t 1: 1 线程, -p 80: FIFO 80, -i 1000: 1ms 间隔, -a 2: CPU 2

# 4. EEVDF 相关 (6.6+)
cat /proc/sys/kernel/sched_features  # 查看调度器特性开关
# latency-nice 通过 sched_setattr() 设置
```

---

## 与旧书差异

| ULK3 讲的 | Bootlin 讲义 |
|-----------|-------------|
| O(1) 调度器 | CFS → EEVDF |
| `runqueue` 全局结构 | `cfs_rq` per-CPU + per-group |
| 无 cgroup 调度 | cgroup v2 CPU 控制是标准实践 |
| 无 latency-nice | latency-nice 是 EEVDF 的延迟控制 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** SCHED_FIFO 和 SCHED_RR 的区别？哪个适合 HFT？

> SCHED_FIFO：同优先级 FIFO，不主动让出则一直运行。SCHED_RR：同优先级轮转，时间片用完后轮到下一个。HFT 交易线程用 SCHED_FIFO（绑定到隔离核，不与其他线程竞争，不需要时间片轮转）。

**Q2:** cgroup v2 的 `cpu.max` 如何限制 CPU？

> 格式 `"$MAX $PERIOD"`：在 PERIOD 微秒内最多用 MAX 微秒。如 `100000 1000000` = 1 秒周期内最多用 0.1 秒 = 10% CPU。这是 CFS bandwidth control，按 cgroup 限制总 CPU 时间。

**Q3:** EEVDF 的 latency-nice 值如何影响调度？

> 低 latency-nice → 更短时间片 → 更近的 virtual deadline → 更频繁被调度。高 latency-nice → 更长时间片 → 更远 deadline → 更少被调度。但相同 nice 值的进程获得相同总量 CPU，只是分片粒度不同。

</details>
