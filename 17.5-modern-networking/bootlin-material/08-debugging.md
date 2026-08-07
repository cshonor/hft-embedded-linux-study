# 08 — 网络调试工具

> **Bootlin 课程模块：** Network Debugging Tools
> **对应 Rosen:** 无

## 收包诊断工具链

| 工具 | 作用 | 层级 |
|------|------|------|
| `ethtool -S` | 网卡统计（rx_drops/rx_errors） | 驱动 |
| `ethtool -c` | 中断合并参数 | 驱动 |
| `nstat` | 内核网络统计（snmp） | 协议栈 |
| `ss` | socket 统计 | socket |
| `dropwatch` | 丢包位置（内核函数级） | 全路径 |
| `tcpdump` | 包捕获 | 协议栈 |
| `perf` | 内核函数级性能分析 | 全路径 |

## dropwatch：定位丢包

```bash
# 交互模式
dropwatch -l kas

# 输出示例
1 drops at tcp_rcv_established+0x1a2 (0xffffffff8c2b3c42)
5 drops at __netif_receive_skb_core+0x89 (0xffffffff8c28a1b9)
```

## ethtool：网卡级诊断

```bash
# 统计信息
ethtool -S eth0 | grep -E "rx_dropped|rx_missed|rx_no_dma"

# 中断合并
ethtool -c eth0

# 调整（减少延迟）
ethtool -C eth0 rx-usecs 0 rx-frames 0

# 队列信息
ethtool -l eth0   # 查看队列数
ethtool -L eth0 combined 4  # 设置队列数

# offload 状态
ethtool -k eth0
```

## BPF 追踪网络延迟

```bash
# 追踪收包延迟（NIC → socket）
bpftrace -e 'tracepoint:net:netif_receive_skb { @start[args->skb] = nsecs; }
tracepoint:net:net_dev_queue { @latency[nsecs - @start[args->skb]] = count(); }'

# 追踪 XDP 程序执行
bpftrace -e 'kprobe:xdp_prog_run { @start[tid] = nsecs; }
kretprobe:xdp_prog_run /@start[tid]/ { printf("XDP: %d ns\n", nsecs - @start[tid]); }'
```

## HFT 延迟诊断流程

```
1. ethtool -S → 网卡有没有丢包/错误
2. dropwatch → 内核哪里丢包
3. nstat → 协议栈层面统计
4. ss -ti → TCP 层面（重传/RTT）
5. bpftrace → 函数级延迟分解
6. perf record → CPU 热点分析
```
