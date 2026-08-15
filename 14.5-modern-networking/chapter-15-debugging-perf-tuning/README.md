# Chapter 15: 调试与性能调优

> 来源：Bootlin（调试 + 性能调优）
> 对标：Rosen（无现代调优工具）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [debugging](notes/01-debugging.md) | Bootlin：网络调试工具、dropwatch、tcpdump、ss |
| 2 | [perf-tuning](notes/02-perf-tuning.md) | Bootlin：网络性能调优、NIC 参数、中断亲缘性 |

## HFT 关联

- **dropwatch**：监控内核网络栈各层丢包计数，定位 HFT 包丢失位置
- **NIC 中断绑核**：`/proc/irq/<irq>/smp_affinity` 将 NIC 中断绑定到特定 core，避免迁移
- **NIC 调优参数**：
  - `ethtool -G eth0 rx 4096 tx 4096`：增大 ring buffer
  - `ethtool -C eth0 rx-usecs 0`：关闭 IRQ coalescing（HFT 要低延迟而非高吞吐）
  - `ethtool -K eth0 gro off tso off`：关闭 GRO/TSO（HFT 发包要即时，不聚合）
- **RPS/RFS**：软件接收包分发，确保包到达正确 CPU core

## 交叉引用

- `16-systems-performance/`：系统级性能调优
- `05.6-kernel-debugging/`：内核调试工具体系
