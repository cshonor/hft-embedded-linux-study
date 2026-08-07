# 02 — Documentation/networking/page_pool.rst

> **对应 Rosen:** Ch1（Rx buffer 分配）
> **内核源码路径:** `Documentation/networking/page_pool.rst`

## 文档概述

page_pool API 官方文档，描述 Rx buffer 的 recycling 池机制。

## 核心内容

### page_pool 工作流程

```
1. 驱动初始化：page_pool_create(&params)
2. 收包：page = page_pool_dev_alloc_pages(pp)
3. DMA 映射：page_pool_get_dma_addr(page)
4. 传递给协议栈（增加引用计数）
5. 协议栈释放：page_pool_put_full_page(pp, page, false) → 回到池中
```

### 关键配置参数

| 参数 | 说明 |
|------|------|
| `pool_size` | 池中 page 数量 |
| `order` | page order（0 = 4KB，1 = 8KB...） |
| `flags` | PP_FLAG_DMA_MAP / PP_FLAG_DMA_SYNC_DEV |
| `nid` | NUMA node 绑定 |
| `dma_dir` | DMA 方向 |

### 与 XDP 的关系

- XDP 程序运行在 page_pool 分配的 page 上
- XDP PASS → page 传递给 sk_buff（不释放）
- XDP DROP → page 直接回收到池中
- AF_XDP → page 映射到用户态 UMEM

### 性能数据

| 场景 | alloc_page | page_pool | 提升 |
|------|-----------|-----------|------|
| 1 Mpps | ~300 cycles/pkt | ~20 cycles/pkt | 15x |
| 10 Mpps | 瓶颈 | 轻松应对 | — |

## HFT 要点

- page_pool 消除收包路径的动态内存分配
- NUMA 绑定确保 Rx buffer 在正确 NUMA node
- page_pool 是 AF_XDP 零拷贝的基础
