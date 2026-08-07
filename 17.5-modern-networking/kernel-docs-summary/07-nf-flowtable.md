# 07 — Documentation/networking/nf_flowtable.rst

> **对应 Rosen:** Ch9（Netfilter）
> **内核源码路径:** `Documentation/networking/nf_flowtable.rst`

## 文档概述

nftables flow table 文档，描述 Netfilter 的硬件流加速机制。

## 核心内容

### Flow Table 是什么

传统 Netfilter 对每个包执行规则匹配，高流量时性能瓶颈。
Flow Table 将已建立连接的流从慢路径（规则匹配）切换到快路径（直接转发）：
- 第一个包走慢路径（完整 Netfilter 规则）
- 建立连接后，流信息缓存到 flow table
- 后续包直接查 flow table，跳过规则匹配

### 工作流程

```
包到达 → 查 flow table
  ├─ 命中 → 快路径（直接转发，O(1)）
  └─ 未命中 → 慢路径（Netfilter 规则匹配）→ 建立 flow entry
```

### 硬件 offload

部分网卡支持将 flow table 卸载到硬件：
- 网卡根据 flow table 直接转发包，不经过 CPU
- 需要网卡驱动支持（mlx5 / ice 等）

## HFT 要点

- HFT 行情流通常不走 Netfilter（已用 XDP 过滤）
- flow table 对非交易流量（管理/监控）的转发加速有用
- 硬件 offload 可减少非交易流量的 CPU 占用
