# HFT 专属抓包场景总览

> [README](../README.md) · [速查](../cheatsheet/notes.md) · [实验指引](../labs/lab-guide.md)

本目录针对 HFT（高频交易）场景补充 Wireshark 的实战用法。基础协议分析见各章笔记，这里聚焦**延迟敏感场景**下 Wireshark 的价值与局限。

## 场景索引

| 文件 | 主题 | 核心问题 |
|------|------|---------|
| [01-tcp-latency-analysis.md](./01-tcp-latency-analysis.md) | TCP 延迟分析 | 重传/RTT/零窗口如何制造延迟尖峰 |
| [02-nic-offload-impact.md](./02-nic-offload-impact.md) | NIC offload 对抓包的影响 | TSO/GRO/GSO 让你看到的是假包 |
| [03-kernel-bypass-limitations.md](./03-kernel-bypass-limitations.md) | 内核旁路与 Wireshark 局限 | DPDK/AF_XDP 场景下 Wireshark 抓不到包 |
| [04-container-cloud-capture.md](./04-container-cloud-capture.md) | 容器/云环境抓包 | K8s overlay/VXLAN/Cilium 场景的抓包技巧 |
| [05-ebpf-vs-wireshark.md](./05-ebpf-vs-wireshark.md) | eBPF 对比 Wireshark | 内核态过滤 vs 用户态分析，各自定位 |

## HFT 延迟敏感点速查

```
交易延迟链路：
  网卡 RX → 内核协议栈 → Socket 队列 → 应用处理 → 下单
                ↑                              ↑
          Wireshark 可观测              Wireshark 不可观测（需 eBPF/ftrace）
```

| 延迟来源 | 典型量级 | Wireshark 能否发现 |
|----------|---------|-------------------|
| TCP 重传 | 200μs–数ms（RTO 级别） | 能，`tcp.analysis.retransmission` |
| 快速重传 | 100μs–1ms | 能，`tcp.analysis.fast_retransmission` |
| 零窗口停滞 | 数ms–数十ms | 能，`tcp.analysis.zero_window` |
| Nagle/Delayed ACK | 40ms（默认） | 能，看包间隔 + PSH 标志 |
| NIC 中断合并 | 50–200μs | 间接，看包时间戳间隔 |
| 内核协议栈处理 | 10–100μs | 不能，需 eBPF/ftrace |
| DPDK 旁路 | <5μs | 不能，Wireshark 完全看不到 |
