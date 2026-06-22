## 6.9 CPU 调优

### 优先级（Gregg 顺序）

1. **消除不必要的工作** — 最高 ROI（Ch 5）
2. 编译器优化（`-O2`/`-O3`、PGO — 需 benchmark）
3. 调度优先级：`nice`、`chrt`（RT 谨慎）
4. 频率：**governor = performance**
5. **CPU 绑定**：`taskset`、`isolcpus`、cpusets
6. **资源控制**：cgroups v2 CPU quota — 云/容器；**HFT 裸机常不用 quota，用隔离**

### 调优手段对照

| 手段 | 命令 / 配置 | HFT 场景 |
|------|-------------|----------|
| **nice** | 降低/提高 CFS 权重 | 监控进程调低 |
| **chrt -f** | SCHED_FIFO 实时 | 仅关键线程 + 文档化 |
| **cpufreq** | `performance` governor | 裸机默认 |
| **taskset** | 绑核 | 进程启动时绑定 |
| **isolcpus** | 内核参数，核不参与通用调度 | 数据面专用核 |
| **cpusets** | cgroup cpuset | 容器化部署 |
| **cgroups CPU** | `cpu.max` quota | 多租户；低延迟共置慎用 |
| **irqbalance 关** | 手动绑 IRQ 到 housekeeping 核 | 网卡 interrupt affinity |
| **RPS/XPS** | 软中断分散 | 与 DPDK 轮询模式互斥 |

**与 Ch 5 衔接：** 应用层伪共享、锁优化 → 这里用 **mpstat + perf** 验证是否真降了 CPU 与 run queue。

---


---

← [本章导读](../README.md)
