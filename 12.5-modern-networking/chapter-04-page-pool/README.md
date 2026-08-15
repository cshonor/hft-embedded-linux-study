# Chapter 04: Page Pool

> 来源：kernel-docs（page pool）+ LWN（page pool 设计）
> 对标：Rosen（无 page pool，3.x 直接 alloc_page）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [page-pool](notes/01-page-pool.md) | kernel-docs：page_pool API、recycle、ring buffer |
| 2 | [page-pool-lwn](notes/02-page-pool-lwn.md) | LWN：page_pool 动机、减少 alloc/free、DMA 映射复用 |

## HFT 关联

- **缓冲区池化**：page_pool 复用已 DMA 映射的 page，避免每次收包都 alloc_page + dma_map
- **延迟降低**：page_pool recycle 使收包路径减少 ~200ns 的页分配开销
- **AF_XDP 共享**：AF_XDP 的 UMEM 基于 page_pool，实现内核-用户态零拷贝共享缓冲区
- **NUMA 亲和**：page_pool 保证缓冲区来自本地 NUMA 节点

## 交叉引用

- `12.5-modern-networking/chapter-02-napi-rx-path/`：NAPI 使用 page_pool 分配接收缓冲区
- `12.5-modern-networking/chapter-06-af-xdp/`：AF_XDP UMEM 基于 page_pool
