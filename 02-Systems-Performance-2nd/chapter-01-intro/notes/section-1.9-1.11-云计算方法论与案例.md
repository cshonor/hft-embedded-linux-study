## 1.9–1.11 云计算、方法论与案例

### 1.9 云计算（HFT 可略读）

- 弹性扩展方便，但带来：**邻居租户干扰**（性能隔离）、物理层观测受限
- **裸机 / 共置 HFT** 通常跳过云章节（[Ch 11](../../chapter-11-cloud-computing/) ⚪），但「noisy neighbor」思想仍适用于 **同机其他进程**

### 1.10 方法论 · 把零散工具串成可复用排障流程

**方法论** = 把前面 **观测 / 实验 / 工具** 从零散技巧变成 **有顺序、可复用** 的流程 — 不用每次「钓鱼式」瞎猜从哪下手。

```
概念（Ch1）→ 清单/USE/RED（Ch2）→ 工具选型（Ch4）→ 分资源深入（Ch6/7/10…）
```

#### Linux 60 秒检查清单（通用 · Gregg）

生产 **第一反应** — 按顺序扫，快速定位 **~90% 常见问题**：

| 命令 | 重点看什么 |
|------|-----------|
| `uptime` | load average 是否异常 |
| `dmesg` | 硬件/OOM/驱动错误 |
| `vmstat 1` | r/b 队列、swap、si/so |
| `mpstat -P ALL 1` | 各 CPU 使用率、是否单核打满 |
| `pidstat 1` | 哪个 PID 吃 CPU |
| `iostat -xz 1` | 磁盘（HFT 热路径通常可忽略） |
| `free -m` | 内存；是否逼近耗尽 |
| `sar -n DEV 1` | 网卡吞吐、错误 |
| `top` / `htop` | 总览 |

→ 完整 USE 清单：[附录 A](../../appendix-A-USE方法Linux.md) · 体系 [Ch 2](../../chapter-02-methodologies/)

#### HFT · 10 秒检查清单（方法论适配实盘）

> **思路：** 60 秒清单偏 **通用服务器**；HFT 热路径 **磁盘常可跳过**，把 time 花在 **网卡 + CPU 亲和 + 热路径** 上。

| 顺序 | 查什么 | 命令 / 动作 | 异常信号 |
|------|--------|-------------|----------|
| **1** | **网卡中断绑核** | `grep eth /proc/interrupts`；`/proc/irq/*/smp_affinity`；`ethtool -S eth0` | IRQ 与策略 **同核**；`rx_missed_errors`、drop |
| **2** | **CPU 亲和 / 负载** | `mpstat -P ALL 1`；`taskset -cp <策略PID>`；`numastat -p <PID>` | 策略核 **%soft 高**；cross-NUMA |
| **3** | **热路径一眼** | `perf top -p <PID>` 或已有 histogram 的 **P99/Max 告警** | 新函数突然变宽；P999 抬升 |

```bash
# HFT 10 秒三连（示例）
grep -E 'eth|enp' /proc/interrupts | head
mpstat -P ALL 1 3
taskset -cp $(pgrep your_strategy)
perf top -p $(pgrep your_strategy)   # 生产用低频/短窗口
```

**10 秒之后往哪走（仍按 Ch1 四层）：**

```
10s 清单异常  →  计数器/Metrics 确认  →  perf 火焰图  →  eBPF trace 抓 tail
                ↑ 与 [1.4 双视角](./section-1.4-热路径Resource与双视角.md) [tick Checklist](./section-1.1-1.3-系统性能角色与活动.md#hft--行情-tick-全链路-checklist) 衔接
```

**方法论落地原则：** 通用清单 **改顺序、删无关项、加 HFT 强相关项** — 形成自己的 **实盘 runbook**，发版/事故 **照表执行**。

### 1.11 案例研究

两则假设案例，演示**用方法论推导根因**（而非猜）：

| 案例 | 表面现象 | 思路 |
|------|----------|------|
| **Slow Disks** | 磁盘慢 | 分层排除：是否真磁盘？是否内存/网络掩盖？ |
| **Software Change** | 变更后变慢 | 对比版本、工作负载 vs 资源、回归基准 |

**HFT 迁移：** 「发版后延迟变差」= Software Change；「偶发尖刺」= 多因并存 + 追踪。

---


---

← [本章导读](../README.md)
