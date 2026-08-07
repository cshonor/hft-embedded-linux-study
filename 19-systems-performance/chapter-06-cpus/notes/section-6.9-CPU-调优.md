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


### 常见陷阱

1. 调优从绑核开始——应先消除不必要工作（ROI 最高），再编译优化，最后才绑核/调度
2. RT 优先级不设上限——SCHED_FIFO 不设 cap 会饿死其他线程，甚至锁死系统
3. cgroup CPU quota 用在裸机——HFT 裸机用隔离（isolcpus）不用 quota，quota 引入 throttling stall

<details>
<summary>自测题（点击展开）</summary>

1. CPU 调优的优先级顺序是什么？
   <details><summary>答</summary>1) 消除不必要工作 2) 编译优化 3) 优先级/nice 4) 频率 governor 5) 绑核 6) cgroup/资源控制</details>
2. SCHED_FIFO 在 HFT 中的风险？
   <details><summary>答</summary>RT 线程不设 cap 会饿死其他线程——需要 rt throttling 兜底（/proc/sys/kernel/sched_rt_runtime_us）</details>
3. HFT 裸机为什么用 isolcpus 而非 cgroup quota？
   <details><summary>答</summary>quota 会 throttling（时间片用完被强制休眠引入 stall），isolcpus 是物理隔离无 throttling</details>

</details>


---

← [本章导读](../README.md)
