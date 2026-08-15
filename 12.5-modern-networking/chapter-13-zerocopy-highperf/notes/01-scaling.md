# 10 — Documentation/networking/scaling.rst（RPS/RFS/XPS）

> **对应 Rosen:** Ch14（高级主题）
> **内核源码路径:** `Documentation/networking/scaling.rst`

## 文档概述

Linux 网络多核扩展技术文档，描述 RPS/RFS/XPS 三种机制。

## 三种扩展机制

### RPS（Receive Packet Steering）

- 软件层将收到的包分发到不同 CPU 的 backlog 队列
- 基于 4-tuple hash 选择目标 CPU
- 不需要网卡硬件支持
- 代价：跨 CPU 传递 sk_buff 开销

### RFS（Receive Flow Steering）

- RPS 的增强版：将包分发到**正在处理该 socket 的 CPU**
- 保持 CPU cache 亲和性
- `rps_flow_table` 记录 flow → CPU 映射
- 需要网卡支持 aRFS（硬件 RFS）

### XPS（Transmit Packet Steering）

- 发包方向：选择特定 CPU 使用特定 Tx 队列
- 减少锁竞争（每个 Tx 队列有锁）
- 通过 `/sys/class/net/eth0/queues/tx-*/xps_cpus` 配置

## 配置

```bash
# RPS：设置可处理 RX 队列 0 的 CPU
echo f > /sys/class/net/eth0/queues/rx-0/rps_cpus

# RFS：全局 flow table 大小
echo 32768 > /proc/sys/net/core/rps_sock_flow_entries

# XPS：CPU 0-3 使用 Tx 队列 0
echo f > /sys/class/net/eth0/queues/tx-0/xps_cpus
```

## vs XDP CPUMAP

| 维度 | RPS/RFS | XDP CPUMAP |
|------|---------|------------|
| 分发时机 | sk_buff 之后 | sk_buff 之前 |
| CPU 开销 | 分配 sk_buff + IPI | 轻量 |
| 亲和性 | RFS 自动 | BPF 自定义 |
| 灵活性 | hash 固定 | BPF 任意逻辑 |

## HFT 要点

- HFT 行情接收：用 XDP CPUMAP 或网卡 RSS（硬件 hash）替代 RPS
- HFT 交易发送：XPS 配置确保发包在交易 CPU 上
- RFS 对 HFT 意义不大（HFT 通常用 AF_XDP/DPDK 绕过协议栈）
