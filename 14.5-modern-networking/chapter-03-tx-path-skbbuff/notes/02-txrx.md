# 08 — Documentation/networking/txrx.rst

> **对应 Rosen:** Ch1（收发包路径）
> **内核源码路径:** `Documentation/networking/txrx.rst`（或相关 driver 文档）

## 文档概述

网卡驱动收发包路径文档，描述 NIC → 内核 → 协议栈的完整数据流。

## 收包路径（RX）

```
1. NIC 收到帧 → DMA 写入 Rx ring 的 buffer（page_pool 分配）
2. NIC 写回 Rx 描述符 → 触发中断
3. 驱动中断处理 → napi_schedule()
4. NAPI 轮询 → napi_poll() → 驱动从 Rx ring 取包
5. 构造 sk_buff（或 XDP 处理）
   ├─ XDP hook（如果有 XDP 程序）
   │   ├─ DROP → page 回收
   │   ├─ PASS → 继续
   │   ├─ REDIRECT → AF_XDP / CPUMAP / DEVMAP
   │   └─ TX → 原路返回
   └─ 无 XDP → 分配 sk_buff
6. napi_gro_receive() → GRO 合并
7. netif_receive_skb() → 协议栈
8. IP 层 → TCP/UDP 层 → socket 队列
```

## 发包路径（TX）

```
1. sendmsg() / sendpage() → 协议栈构造 sk_buff
2. qdisc 排队（fq / pfifo_fast）
3. 驱动 dequeue → 映射 DMA → 写入 Tx ring
4. NIC DMA 发送
5. NIC 完成中断 → 驱动清理 Tx ring → 释放 sk_buff
```

## 现代驱动关键点

| 组件 | 传统 | 现代 |
|------|------|------|
| Rx buffer | alloc_page | page_pool |
| XDP | 无 | native XDP hook |
| GRO | 软件 | 软件 + 硬件 offload |
| TSO | 软件 | 硬件 TSO |
| 中断合并 | 无 | ethtool -C 可调 |

## HFT 要点

- 理解完整 RX 路径是延迟优化的基础
- 每一步都可能引入延迟：中断合并、GRO、协议栈处理、socket 唤醒
- XDP 在步骤 5 最早期处理，是减少延迟的关键
