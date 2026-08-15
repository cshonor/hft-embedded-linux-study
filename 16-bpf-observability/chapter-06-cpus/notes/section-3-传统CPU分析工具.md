# 3. 传统 CPU 分析工具

**先传统、后 BPF** — [Ch 3 § Linux 60 秒](../../chapter-03-performance-analysis/) 已列；本章补充 CPU 专项：

### 系统状态与利用率

| 工具 | 看什么 |
|------|--------|
| `uptime` | load average — 可运行 + 不可中断任务压力 |
| `top` / `htop` | 整体 %us/%sy、 per-process CPU |
| `mpstat -P ALL 1` | **每核** 利用率 — 发现单核打满、不均衡 |
| `pidstat -p PID -u 1` | 单进程 CPU 随时间变化 |

```bash
mpstat -P ALL 1
pidstat -u -p $(pidof my_strategy) 1
```

### perf 与 PMC

| 用途 | 示例 |
|------|------|
| 采样剖析 | `perf record -F 99 -a -g -- sleep 30` |
| 硬件计数 | `perf stat -e cache-misses,cycles,instructions` |
| IPC | instructions / cycles — 低 IPC 常暗示缓存/分支问题 |

### CPU 火焰图

```
perf record -F 99 -a -g -- sleep 30
perf script | stackcollapse-perf.pl | flamegraph.pl > cpu.svg
```

**要点：** 采样频率常用 **49Hz / 99Hz** — 避免与内核 tick 锁步；宽度 = 该栈占样本比例。

→ 栈与火焰图原理：[Ch 2 § 火焰图](../../chapter-02-technology-background/) · BCC 等价：`profile-bpfcc`


### 常见陷阱

1. **只依赖 top/htop 做 CPU 分析** — top 只显示聚合利用率，看不到函数级热点和等待原因；应配合 perf（采样）和 BPF（精确追踪）使用
2. **忽视 mpstat 的 per-CPU 维度** — mpstat -P ALL 能看每个 CPU 核的利用率分布；如果某核 100% 而其他空闲，可能是单线程瓶颈或中断集中在某核
3. **用 vmstat 看 CPU 问题时忽视 run queue 长度** — vmstat 的 r 列是 run queue 长度，r > CPU 核数说明有饱和；HFT 应关注 r 是否有偶发性尖峰

<details>
<summary>📝 自测题（点击展开）</summary>

1. **传统 CPU 分析工具有哪些？各自能看什么？**

   <details>
   <summary>参考答案</summary>

   (1) top/htop：进程级 CPU 利用率（粗粒度）；(2) mpstat -P ALL：per-CPU 利用率（用户/系统/IO 等待/空闲）；(3) vmstat：系统级 CPU+内存+IO 概览（r 列=run queue）；(4) pidstat：per-进程 CPU/内存/IO；(5) perf stat/top/record：硬件计数器+调用栈采样。

   </details>

2. **传统工具相比 BPF 的盲区有哪些？**

   <details>
   <summary>参考答案</summary>

   (1) 极短命进程（top 采样不到→需 execsnoop）；(2) 运行队列等待延迟（mpstat 只见忙闲→需 runqlat 直方图）；(3) off-CPU 原因（perf 默认 on-CPU→需 offcputime）；(4) per-进程 LLC 命中率（perf stat 粗粒度→需 llcstat）；(5) 精确事件级追踪（传统工具无→需 bpftrace/BCC trace）。

   </details>

3. **HFT 排障中如何组合使用传统工具和 BPF 工具？**

   <details>
   <summary>参考答案</summary>

   第一步（快速概览）：mpstat -P ALL + vmstat 看系统级异常（某核 100%、r 尖峰）。第二步（定位范围）：pidstat 看哪个进程异常。第三步（深度分析）：BPF 工具钻取——runqlat 看排队延迟分布、offcputime 看等待原因、profile 采样看热点函数。传统工具做「有没有问题」，BPF 做「为什么有问题」。

   </details>

</details>

---
