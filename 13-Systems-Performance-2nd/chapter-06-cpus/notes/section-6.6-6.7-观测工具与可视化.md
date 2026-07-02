## 6.6–6.7 观测工具与可视化

### 传统系统工具

| 工具 | 看什么 | HFT 用法 |
|------|--------|----------|
| **`uptime`** | **Load average**（1/5/15 min） | 粗筛；**不能替代** per-CPU 分析 |
| **PSI** | `/proc/pressure/cpu` **压力停滞** | Linux 4.20+；比 load 更反映「等 CPU」 |
| **`vmstat 1`** | `r` 运行队列、`b` 阻塞、`cs` 上下文切换 | 尖刺时先看 `r` 与 `cs` |
| **`mpstat -P ALL 1`** | **每 CPU** user/sys/idle/irq | 找热点核、是否单核打满 |
| **`pidstat -u -w 1`** | 按进程/线程 CPU + **cswch** | 定位哪个 TID 切换多 |
| **`top` / `htop`** | 实时排序 | 危机第一轮 |

**Load Average 大白话：**

- 可运行 + 不可中断睡眠（如 D 态 I/O）线程数的指数平均 — **不是 CPU 使用率**。
- 8 核机器 load 8 ≠ 100% — 要看 **mpstat** 每核分布。

**PSI 示例：**

```bash
cat /proc/pressure/cpu
# some avg10=0.00 avg60=...  →  部分线程因 CPU 不足而stall 的时间占比
```

### perf 与 BPF 工具集

| 工具 | 作用 |
|------|------|
| **`perf stat`** | IPC、PMC、整体计数 |
| **`perf record` / `report`** | CPU 剖析、调度 trace |
| **`profile`（BCC）** | BPF 栈采样 |
| **`runqlat`** | run queue **延迟分布** — 调度饱和度金标准 |
| **`runqlen`** | run queue **长度** 随时间 |
| **`softirqs` / `hardirqs`** | 中断 CPU 消耗 — 网卡收包路径 |

```bash
# 调度延迟（生产限时长）
sudo runqlat-bpfcc 10

# 每 CPU 使用率
mpstat -P ALL 1 5
```

→ [Ch 15 BPF](../../chapter-15-bpf/) · [附录 C](../../appendix-C-bpftrace单行命令.md) · [14-BPF](../../../14-BPF-Performance-Tools/)

### 可视化：火焰图与 FlameScope

**CPU 火焰图（Flame Graph）：**

- X 轴：**样本占比**（非时间顺序）；Y 轴：栈深度。
- **最宽的塔** = 最热函数 — 自顶向下读「谁调用谁」。

**FlameScope：**

- **亚秒级偏移热力图** + 可选火焰图 — 在大样本里找 **抖动、方差、周期性尖刺**。
- HFT：把 **P99 延迟尖刺** 的时间窗口对齐到 FlameScope，看是 GC、调度还是 IRQ。

→ Ch 2 [FlameScope / 热力图](../../chapter-02-methodologies/) · Ch 1 [火焰图概念](../../chapter-01-intro/)

---


---

← [本章导读](../README.md)
