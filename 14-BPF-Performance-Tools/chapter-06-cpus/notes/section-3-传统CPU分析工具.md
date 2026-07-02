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

---
