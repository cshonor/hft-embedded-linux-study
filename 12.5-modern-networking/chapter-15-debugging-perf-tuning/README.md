# Chapter 15: 调试与性能调优

> 来源：Bootlin（调试 + 性能调优）
> 对标：Rosen（无现代调优工具）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [debugging](notes/01-debugging.md) | Bootlin：网络调试工具、dropwatch、tcpdump、ss |
| 2 | [perf-tuning](notes/02-perf-tuning.md) | Bootlin：网络性能调优、NIC 参数、中断亲缘性 |
| 3 | [latency-measurement](notes/03-latency-measurement.md) | 延迟测量方法论：时钟选择（CLOCK_MONOTONIC_RAW / rdtsc）、分位直方图、误差源清单、报告模板 |

## HFT 关联

- **dropwatch**：监控内核网络栈各层丢包计数，定位 HFT 包丢失位置
- **NIC 中断绑核**：`/proc/irq/<irq>/smp_affinity` 将 NIC 中断绑定到特定 core，避免迁移
- **NIC 调优参数**：
  - `ethtool -G eth0 rx 4096 tx 4096`：增大 ring buffer
  - `ethtool -C eth0 rx-usecs 0`：关闭 IRQ coalescing（HFT 要低延迟而非高吞吐）
  - `ethtool -K eth0 gro off tso off`：关闭 GRO/TSO（HFT 发包要即时，不聚合）
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
