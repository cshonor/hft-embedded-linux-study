# 07 — XDP redirect 与 cpumap

> **对应 Rosen:** 无
> **内核版本:** XDP redirect 4.8+；CPUMAP 4.15+；DEVMAP 4.14+

## XDP redirect 概述

XDP 程序可以将包重定向到不同目标，而非简单 PASS/DROP：
- **CPUMAP**：将包分发到特定 CPU 核心处理
- **DEVMAP**：将包转发到另一个网卡
- **AF_XDP socket**：将包送到用户态
- **BPF map（全局）**：跨程序/跨 CPU 传递包

## CPUMAP：CPU 亲和性收包分发

传统 RPS 在软件层分发包到不同 CPU，但需要先分配 sk_buff。
CPUMAP 在 XDP 层（sk_buff 之前）分发：

```c
// BPF 程序：按 hash 将包分发到不同 CPU
SEC("xdp")
int xdp_cpumap(struct xdp_md *ctx) {
    void *data = (void *)(long)ctx->data;
    // 解析包头部计算 hash
    uint32_t cpu = hash % NUM_CPUS;
    return bpf_redirect_map(&cpumap, cpu, 0);  // XDP_REDIRECT
}
```

CPUMAP 的工作原理：
1. XDP 程序调用 `bpf_redirect_map()` 将包放入目标 CPU 的队列
2. 目标 CPU 的 kthread 从队列取出包
3. 构造 sk_buff，送入协议栈（或再次通过 BPF 处理）

| 特性 | RPS（传统） | CPUMAP（XDP） |
|------|------------|---------------|
| 分发时机 | sk_buff 分配后 | sk_buff 分配前 |
| CPU 开销 | 需分配 sk_buff | 轻量，延迟分配 |
| 灵活性 | hash 策略固定 | BPF 程序自定义 |

## DEVMAP：网卡间转发

```c
SEC("xdp")
int xdp_redirect_dev(struct xdp_md *ctx) {
    return bpf_redirect_map(&devmap, target_ifindex, 0);
}
```
- 可实现内核态 L2 转发（替代部分 bridge 功能）
- 性能远高于传统转发（不经过协议栈）

## HFT 关联

| 场景 | redirect 类型 | 用途 |
|------|-------------|------|
| 行情多核分发 | CPUMAP | 不同行情流分发到不同 CPU，各自独立处理 |
| 行情镜像 | DEVMAP | 将行情复制转发到监控/录制服务器 |
| 行情旁路 | AF_XDP | 将行情包送到用户态交易进程 |
| 跨网卡转发 | DEVMAP | 交易报文从内网网卡转发到交易所网卡 |
