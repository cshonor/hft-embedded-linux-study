# 2. CPU 基础知识 (Background)

### CPU 模式

| 模式 | 说明 | 传统工具中的体现 |
|------|------|------------------|
| **用户态** | 应用代码 | `top` 的 `%us` |
| **内核态** | 系统调用、驱动、协议栈 | `%sy` |
| **空闲 / iowait / steal** | 等 I/O、虚拟化偷跑等 | `%id`、`%wa`、`%st` |

**HFT：** 策略热路径应 **大部分在用户态**；`%sy` 突增 → 查 syscall 风暴或内核网络栈（衔接 [Ch 10 网络](../../chapter-10-networking/)）。

### CPU 调度器与线程状态

调度器在 **任务（线程）** 之间分配 CPU 时间片：

| 状态 | 含义 | BPF 相关 |
|------|------|----------|
| **ON-CPU** | 正在某核上运行 | `profile`、`cpudist` |
| **RUNNABLE** | 就绪，在 **运行队列** 等 CPU | `runqlat`、`runqlen`、`runqslower` |
| **SLEEP** | 阻塞（I/O、锁、futex…） | `offcputime` |

→ 内核实现对照：[05-linux-kernel Ch 4 调度](../../../../05-linux-kernel/chapter-04-process-scheduling/)

### CPU 缓存与 TLB

现代负载常为 **内存/缓存密集型**，不单看 GHz：

| 层级 | 作用 |
|------|------|
| **L1 / L2** |  per-core，最快 |
| **L3 (LLC)** | 末级缓存，多核共享 |
| **TLB** | 虚拟地址 → 物理页表项缓存 |

**工具：** `perf` PMC、`llcstat`（BPF + 硬件计数）看 LLC 命中/未命中 — 与 [CSAPP Ch6 存储层次](../../../../02-computer-systems/chapter-06-memory-hierarchy/) 对照。

→ SysPerf CPU 章：[chapter-06-cpus](../../../../14-systems-performance/chapter-06-cpus/)


### 常见陷阱

1. **混淆 CPU 核数和硬件线程数** — 一个物理核可支持超线程（SMT），2 个硬件线程共享一个物理核的执行单元；HFT 通常关闭 SMT 以避免资源争用
2. **忽视 NUMA 对延迟的影响** — 跨 NUMA 节点访问内存比本地节点慢 30-50%；HFT 应确保线程和内存在同一 NUMA 节点（numactl --membind）
3. **混淆 CPU 频率缩放和功耗管理** — CPU 频率动态调节（cpufreq/governor）会导致指令执行速度变化；HFT 应锁定最高频率（performance governor）避免 DVFS 抖动

<details>
<summary>📝 自测题（点击展开）</summary>

1. **物理核、硬件线程（SMT）、NUMA 节点的关系是什么？HFT 如何配置？**

   <details>
   <summary>参考答案</summary>

   一个物理核可支持超线程（SMT），2 个硬件线程共享执行单元。多核 CPU 分为多个 NUMA 节点，跨节点访问内存更慢。HFT 配置：(1) 关闭 SMT（避免资源争用）；(2) 用 isolcpus 隔离关键核；(3) 用 numactl 绑定线程和内存到同一 NUMA 节点；(4) 锁定 CPU 频率（performance governor）。

   </details>

2. **为什么 HFT 要关闭 CPU 频率缩放（DVFS）？**

   <details>
   <summary>参考答案</summary>

   DVFS（动态电压频率调节）会根据负载改变 CPU 频率——低负载时降频省电，但频率切换有延迟（微秒级），导致指令执行速度不稳定。HFT 要求每条指令的执行时间可预测，应设 `cpufreq governor = performance` 锁定最高频率，消除 DVFS 抖动。

   </details>

3. **CPU 迁移（migration）对 HFT 有什么影响？如何避免？**

   <details>
   <summary>参考答案</summary>

   线程从 CPU A 迁移到 CPU B 时，L1/L2/L3 cache 全部 miss，需要重新预热，导致数百纳秒到微秒级的额外延迟。避免方法：(1) `taskset -c N` 绑定到固定核；(2) `isolcpus=` 内核参数隔离关键核；(3) 关闭 `CONFIG_NO_HZ_FULL` 之外的定时器中断；(4) 设置 `sched_setaffinity`。

   </details>

</details>

---
