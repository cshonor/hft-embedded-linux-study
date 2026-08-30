# Chapter 06: AF_XDP

> 来源：kernel-docs（AF_XDP API）+ LWN（AF_XDP 设计）
> 对标：Rosen（无 AF_XDP，3.x 不存在）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [af-xdp](notes/01-af-xdp.md) | kernel-docs：AF_XDP socket、UMEM、fill/tx/rx/completion ring |
| 2 | [af-xdp-lwn](notes/02-af-xdp-lwn.md) | LWN：AF_XDP 零拷贝模式、XDP_REDIRECT 到 socket |
| 3 | [af-xdp-umem-layout](notes/03-af-xdp-umem-layout.md) | UMEM frame 布局、FILL/RX ring 所有权交接、copy vs zero-copy 微观差异与量化对比 |

## HFT 关联

- **AF_XDP 零拷贝**：XDP_REDIRECT 将 NIC DMA 的包直接放入用户态 UMEM，零拷贝收包到用户态
- **HFT 收包主力**：AF_XDP 是内核态零拷贝收包的最佳方案，延迟 ~200-500ns（DPDK ~100ns 但完全 bypass 内核）
- **UMEM 共享**：UMEM 内存区域在内核与用户态之间共享，pre-registered 避免 mmap 开销
- **bind 到特定 queue**：AF_XDP socket 绑定到特定 RX queue，实现 per-core 收包
- **只认 zero-copy**：copy 模式仍分配 sk_buff 且多一次 memcpy，相对 `recvmsg` 优势有限
  → [03](notes/03-af-xdp-umem-layout.md)
- **FILL ring 水位是生命线**：`rx_dropped` 增长 = frame 归还太慢 → [03](notes/03-af-xdp-umem-layout.md)

## 交叉引用

- `12.5-modern-networking/chapter-04-page-pool/`：UMEM 基于 page_pool
- `12.5-modern-networking/chapter-05-xdp-architecture/`：XDP 架构，AF_XDP 是 XDP redirect 目标
- `12.5-modern-networking/chapter-02-napi-rx-path/notes/07-queue-steering-rss.md`：AF_XDP 按队列旁路，需先用 ntuple 把行情流钉到独占队列
- `13-dpdk/`：DPDK 完全 bypass，AF_XDP 是内核态折中
