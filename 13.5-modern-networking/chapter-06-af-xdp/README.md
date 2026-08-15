# Chapter 06: AF_XDP

> 来源：kernel-docs（AF_XDP API）+ LWN（AF_XDP 设计）
> 对标：Rosen（无 AF_XDP，3.x 不存在）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [af-xdp](notes/01-af-xdp.md) | kernel-docs：AF_XDP socket、UMEM、fill/tx/rx/completion ring |
| 2 | [af-xdp-lwn](notes/02-af-xdp-lwn.md) | LWN：AF_XDP 零拷贝模式、XDP_REDIRECT 到 socket |

## HFT 关联

- **AF_XDP 零拷贝**：XDP_REDIRECT 将 NIC DMA 的包直接放入用户态 UMEM，零拷贝收包到用户态
- **HFT 收包主力**：AF_XDP 是内核态零拷贝收包的最佳方案，延迟 ~200-500ns（DPDK ~100ns 但完全 bypass 内核）
- **UMEM 共享**：UMEM 内存区域在内核与用户态之间共享，pre-registered 避免 mmap 开销
- **bind 到特定 queue**：AF_XDP socket 绑定到特定 RX queue，实现 per-core 收包

## 交叉引用

- `13.5-modern-networking/chapter-04-page-pool/`：UMEM 基于 page_pool
- `13.5-modern-networking/chapter-05-xdp-architecture/`：XDP 架构，AF_XDP 是 XDP redirect 目标
- `14-dpdk/`：DPDK 完全 bypass，AF_XDP 是内核态折中
