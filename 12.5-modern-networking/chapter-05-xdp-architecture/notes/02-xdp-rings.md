# 03 — Documentation/networking/xdp-rings-design.rst

> **对应 Rosen:** 无
> **内核源码路径:** `Documentation/networking/xdp-rings-design.rst`

## 文档概述

XDP 环形缓冲区设计文档，描述 AF_XDP socket 的 UMEM 和 ring 数据结构。

## 核心内容

### 四个 Ring

| Ring | 生产者 | 消费者 | 作用 |
|------|--------|--------|------|
| FILL | 用户态 | 内核态 | 用户态提供空闲 buffer |
| RX | 内核态 | 用户态 | 内核通知收到的包 |
| TX | 用户态 | 内核态 | 用户态提交待发送包 |
| COMPLETION | 内核态 | 用户态 | 内核通知发送完成 |

### UMEM（共享内存）

- 用户态分配的大块连续内存
- 切分为固定大小的 chunk（通常 4KB）
- 通过偏移量（offset）在 ring 中传递
- 零拷贝模式下 page_pool 的 page 映射到 UMEM

### Ring 同步

- 生产者写 `producer` 指针，消费者读
- 消费者写 `consumer` 指针，生产者读
- 使用 `smp_wmb()` / `smp_rmb()` 保证内存序
- 用户态和内核态共享同一块映射内存

## HFT 要点

- FILL ring 需要预填充足够 buffer，否则收包丢包
- TX + COMPLETION 用于双向零拷贝（行情接收 + 交易发送）
- ring 大小影响丢包率：太小在高 PPS 下溢出
