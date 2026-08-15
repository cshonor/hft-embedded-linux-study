# 09 — 性能调优

> **Bootlin 课程模块：** Network Performance Tuning
> **对应 Rosen:** Ch14

## 调优清单

### 网卡参数（ethtool）

```bash
# 关闭 GRO/TSO（降低延迟）
ethtool -K eth0 gro off tso off

# 中断合并调零（最低延迟）
ethtool -C eth0 rx-usecs 0 rx-frames 0 tx-usecs 0 tx-frames 0

# 设置队列数
ethtool -L eth0 combined 4

# RSS（硬件多队列 hash）
ethtool -X eth0 equal 4
```

### 内核参数（sysctl）

```bash
# 网络缓冲区
sysctl -w net.core.rmem_max=33554432
sysctl -w net.core.wmem_max=33554432
sysctl -w net.core.rmem_default=262144
sysctl -w net.core.wmem_default=262144

# backlog
sysctl -w net.core.netdev_max_backlog=10000

# TCP 调优
sysctl -w net.ipv4.tcp_no_metrics_save=1
sysctl -w net.ipv4.tcp_low_latency=1  # (已弃用，参考意义)
```

### CPU 亲和性

```bash
# 网卡中断绑定到特定 CPU
# 查看 IRQ 号
cat /proc/interrupts | grep eth0

# 绑定
echo <cpu_mask> > /proc/irq/<irq>/smp_affinity

# NAPI 线程绑定
taskset -c 2 $(pgrep "napi/eth0")
```

### HFT 综合调优清单

| 项目 | 设置 | 目的 |
|------|------|------|
| GRO | off | 减少合并延迟 |
| TSO | off | 降低发送延迟 |
| 中断合并 | 0 | 最低收包延迟 |
| RPS/RFS | off（用 RSS/XDP 替代） | 避免软件分发开销 |
| SO_BUSY_POLL | 50-100 μs | 跳过中断唤醒 |
| CPU 隔离 | isolcpus + irq affinity | 交易 CPU 独占 |
| NUMA 绑定 | numactl --cpunodebind --membind | 避免跨节点访问 |
| QDisc | pfifo_fast 或 noop | 减少排队延迟 |

### 树莓派 5 特殊注意事项

- 树莓派 5 网卡（BCM2712）支持 XDP generic 模式
- AF_XDP 零拷贝模式需要内核 5.x+ 驱动支持
- 中断亲和性需通过 `/proc/irq/` 手动配置
- 无硬件 TSO/GRO offload（纯软件），性能低于服务器网卡
