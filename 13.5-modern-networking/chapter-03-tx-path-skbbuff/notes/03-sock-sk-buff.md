# 09 — sk_buff 生命周期

> **对应 Rosen:** Ch1/Ch11（sk_buff 是核心数据结构）
> **内核源码路径:** `Documentation/networking/` 相关 + `include/linux/skbuff.h`

## sk_buff 概述

sk_buff（socket buffer）是 Linux 网络栈的核心数据结构：
- 包含包数据指针 + 元数据（协议头偏移、队列映射、时间戳等）
- 在协议栈各层之间传递
- 分配/释放是网络栈的主要开销之一

## 关键字段

| 字段 | 作用 | HFT 关注 |
|------|------|---------|
| `head` / `data` / `tail` / `end` | 数据缓冲区指针 | 数据访问 |
| `len` / `data_len` | 包长度 | — |
| `protocol` | L3 协议 | 解析 |
| `pkt_type` | 包类型（unicast/multicast/broadcast） | 行情组播 |
| `mark` | skb 标记 | tc-BPF 设置 |
| `priority` | 优先级 | qdisc |
| `queue_mapping` | 网卡队列映射 | 多核收包 |
| `tstamp` / `ktstamp` | 时间戳 | 延迟测量 |
| `cb` | 控制块（每层自定义） | 传递元数据 |
| `skb->users` | 引用计数 | 生命周期 |

## 分配/释放路径

```
alloc_skb() → kmem_cache_alloc(skbuff_head_cache)
  → 分配 sk_buff 结构体（~256 字节）
  → 分配数据缓冲区（__alloc_skb / page_pool）

kfree_skb() → 引用计数减 1
  → 引用计数 = 0 → 释放数据缓冲区 + 释放 sk_buff
```

## XDP 对 sk_buff 的影响

| XDP 动作 | sk_buff | 延迟 |
|----------|---------|------|
| XDP_DROP | 不分配 | 最少 |
| XDP_PASS | 分配（延迟到 XDP 之后） | 减少 |
| XDP_REDIRECT (AF_XDP) | 不分配 | 最少 |
| 无 XDP | 立即分配 | 传统 |

## HFT 要点

- sk_buff 分配开销约 ~100-200 cycles，高 PPS 场景累积显著
- XDP DROP 路径避免 sk_buff 分配，是性能关键
- `skb->tstamp` 可用于测量协议栈各段延迟
- `skb->mark` 配合 tc qdisc 实现流量优先级
