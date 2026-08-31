# Chapter 15: 调试与性能调优

> 来源：Bootlin（调试 + 性能调优）
> 对标：Rosen（无现代调优工具）
> 本版基于 **v6.6 源码核验**重写（trace_skb.h / trace_net.h / dev.c 锚点），修正旧版 3 处错误依据

## 核心结论（全章浓缩）

1. **v6.6 丢包定位标准姿势是 `kfree_skb` tracepoint + drop reason**（trace_skb.h:24，`enum skb_drop_reason` 符号化输出）——dropwatch 只是同源事件的旧封装；perf/bpftrace 能同时拿到 location 与 reason。
2. **bpftrace tracepoint 字段以 `format` 文件为准**：`netif_receive_skb`/`net_dev_queue` 的字段是 `skbaddr`（net.h:23），不是旧笔记编造的 `args->skb`；XDP 计时的 kprobe 目标是 `bpf_prog_run_xdp()`（dev.c:4887）。
3. **四层计数器先跑再上重武器**：ethtool -S（驱动）→ softnet_stat（softirq）→ nstat（协议）→ ss（socket）——层间差值直接圈定丢包位置，零观测开销。
4. **低延迟第一杠杆是中断合并**（rx-usecs 直接加在尾延迟上）；**"关 GRO/TSO 降延迟"是错误依据**（ch14 已证伪：GRO 无等待窗、TSO 不等凑包）——关它们的真实理由是行为可预测性，且要付吞吐代价。
5. **调优按流量画像分口**：交易口 `pfifo limit 256` + 中断清零 + busy poll + 单核隔离（nohz_full + rcu_nocbs 成对 + cpuset partition + irqbalance 必停）；转发口 GRO on + fq（EDT pacing 执行者）。
6. **树莓派 5 网卡是 RP1 南桥上的 Cadence GEM（`macb` 驱动）+ BCM54213 PHY**——不是 BCM2712（那是 SoC），也不是 Pi 4 的 bcmgenet；单队列 GbE 决定多队列范式在 Pi 上练不了，但方法论（合并清零/busy poll/隔离/量测闭环）完全同构。

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [debugging](notes/01-debugging.md) | 四层计数器、kfree_skb drop reason、bpftrace 修正版、诊断决策树 |
| 2 | [perf-tuning](notes/02-perf-tuning.md) | 中断合并/offload 真相、sysctl、qdisc、CPU/NUMA 隔离、树莓派 5 修正 |
| 3 | [latency-measurement](notes/03-latency-measurement.md) | 延迟测量方法论：时钟选择（CLOCK_MONOTONIC_RAW / rdtsc）、分位直方图、误差源清单、报告模板 |

## HFT 关联

- **dropwatch**：监控内核网络栈各层丢包计数，定位 HFT 包丢失位置
- **NIC 中断绑核**：`/proc/irq/<irq>/smp_affinity` 将 NIC 中断绑定到特定 core，避免迁移
- **NIC 调优参数**：
  - `ethtool -G eth0 rx 4096 tx 4096`：增大 ring buffer
  - `ethtool -C eth0 rx-usecs 0`：关闭 IRQ coalescing（**低延迟第一杠杆**，合并窗直接加在尾延迟上）
  - GRO/TSO：交易口关 GRO 是为**行为可预测**（非"省合并延迟"——GRO 无等待窗）；TSO 保持开（不等凑包，关掉无收益，见 ch14-01/02）
- **RPS/RFS：HFT 应关闭。** 它是为多核吞吐设计的软件分发，会引入 IPI 与额外排队，
  只增加延迟抖动；有硬件多队列时纯负收益。改用 RSS / ntuple 硬件分发
  → [02 HFT 综合调优清单](notes/02-perf-tuning.md)、[ch02/06-queue-steering-rss](../chapter-02-napi-rx-path/notes/06-queue-steering-rss.md)
- **延迟必须看分位数**：p999 才是考核线，均值会掩盖所有导致亏损的尾延迟
  → [03-latency-measurement](notes/03-latency-measurement.md)

## 交叉引用

- `06.6-systems-performance/`：系统级性能调优
- `05.6-kernel-debugging/`：内核调试工具体系
- `14-hft-engineering/chapter-09-latency-measurement-benchmarking/`：本 ch03 是它的实操方法基础
- `projects/P10-hft-prototype/docs/benchmark.md`：用 ch03 的报告模板落地延迟数据
