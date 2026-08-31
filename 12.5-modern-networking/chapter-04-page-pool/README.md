# Chapter 04: Page Pool

> 来源：kernel-docs（page pool）+ LWN（page pool 设计）
> 对标：Rosen（无 page pool，3.x 直接 alloc_page）
> 内核版本：以 v6.6 为准，API 签名与常量均已核对源码

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [page-pool](notes/01-page-pool.md) | **机制本体**：三层结构、分配/释放 API、refcnt 判定规则、frag 模式、观测 |
| 2 | [page-pool-lwn](notes/02-page-pool-lwn.md) | **工程实践**：为什么需要它、容量模型、HFT 调优清单、五个陷阱 |

## 本篇的四个核心结论

1. **最大的一笔收益是 `PP_FLAG_DMA_MAP` 的保持映射**，不是"避免 `alloc_page`"。
   有 IOMMU 时每次 `dma_map` 可达数百 ns，page_pool 把它完全消除（映射一次、长期保持）。
2. **`pool_size` 管的不是快缓存**。per-CPU 快缓存是**硬编码的 128**（`PP_ALLOC_CACHE_SIZE`），
   不可调；`pool_size` 决定的是第二层 **ptr_ring** 的大小（默认 1024，**上限 32768**）。
3. **回收判定全靠 refcnt**：refcnt == 1 → 回收进池；refcnt > 1 → unmap 并真释放回伙伴系统。
   所有 page_pool 相关的"性能莫名劣化"都源于这条规则。
4. **AF_XDP 零拷贝不用 page_pool**（用用户态 UMEM，`MEM_TYPE_XSK_BUFF_POOL`）。
   说"AF_XDP 基于 page_pool"只在 **copy 模式**下成立。

## HFT 关联

- **缓冲区池化**：page_pool 复用已 DMA 映射的 page，避免每次收包都 `alloc_page` + `dma_map`
- **延迟降低**：消除收包路径上的伙伴系统往返与 DMA 映射开销（具体数值随 IOMMU/配置差异很大，**需自测**）
- **NUMA 亲和**：`page_pool_params.nid` 指定 page 来源节点——⚠️ 应设成**网卡所在**节点，
  不是当前线程所在节点（与 DPDK `rte_eth_dev_socket_id()` 同一类坑）
- **frag 模式**：`PP_FLAG_PAGE_FRAG` 一页多段，行情小包场景必需（4 KB 页给 64 B 包浪费 98%）
- **XDP_TX 要用 `DMA_BIDIRECTIONAL`**：`PP_FLAG_DMA_MAP` 下只接受 `DMA_FROM_DEVICE`
  和 `DMA_BIDIRECTIONAL`，前者做不了原路反弹发送
- **tcpdump 会抽干 page_pool**：AF_PACKET 克隆 skb → refcnt 抬高 → 页面永久离池。
  **排障工具本身是观测扰动源**
- **运行中别改 ring 大小 / 队列数**：驱动 reload 会重建 pool，页面全丢，之后延迟明显升高

## 交叉引用

- `12.5-modern-networking/chapter-02-napi-rx-path/`：NAPI 使用 page_pool 分配接收缓冲区
- `12.5-modern-networking/chapter-03-tx-path-skbbuff/`：page_pool 的页如何零拷贝变成 skb
- `12.5-modern-networking/chapter-05-xdp-architecture/`：XDP 依赖 page_pool 的回收语义
- `12.5-modern-networking/chapter-06-af-xdp/`：AF_XDP 零拷贝用 UMEM，与 page_pool 是两套体系
