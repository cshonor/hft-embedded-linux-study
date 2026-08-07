# 02 — page_pool API：现代 Rx buffer 管理

> **对应 Rosen:** Ch1/Ch4（收包路径中 buffer 分配）
> **内核版本:** 4.18+（page_pool 核心库），广泛采用于 5.x 驱动

## 问题背景

传统收包路径中，每个 Rx buffer 需要分配一个 page：
- `alloc_page()` → 映射 DMA → 填充数据 → 传递给协议栈
- page 使用完后 `put_page()` 释放，下次收包再 `alloc_page()`
- 高 PPS 场景下，page 分配/释放成为瓶颈

## page_pool 解决方案

page_pool 是一个**页 recycling 池**：
- 预分配一批 page，收包时从池中取
- page 传递给协议栈时增加引用计数
- 协议栈释放 page 时回到池中（而非真正释放）
- 避免反复 `alloc_page()` / `put_page()`

```c
struct page_pool *pp;
struct page_pool_params params = {
    .order = 0,
    .flags = PP_FLAG_DMA_MAP,
    .pool_size = 256,
    .nid = NUMA_NO_NODE,
    .dev = &pdev->dev,
    .dma_dir = DMA_FROM_DEVICE,
};
pp = page_pool_create(&params);

// 收包时
page = page_pool_dev_alloc_pages(pp);
dma_addr = page_pool_get_dma_addr(page);

// 释放时（协议栈持有完毕）
page_pool_put_full_page(pp, page, false);
```

## 现代驱动采用情况

| 驱动 | 是否使用 page_pool | 内核版本 |
|------|-------------------|---------|
| mlx5（Mellanox） | 是 | 5.x+ |
| ice（Intel E810） | 是 | 5.x+ |
| stmmac（树莓派 5 网卡） | 是 | 5.x+ |
| virtio-net | 是 | 5.x+ |

## 与 XDP 的协同

page_pool 是 XDP 的基础设施：
- XDP 程序在 page_pool 分配的 page 上运行
- AF_XDP 零拷贝路径直接使用 page_pool 的 page
- XDP redirect 可以传递 page_pool 的 page 到其他设备

## HFT 关联

| 维度 | HFT 影响 |
|------|---------|
| 内存分配延迟 | 消除收包路径中的 alloc_page 开销 |
| NUMA 亲和性 | page_pool 可绑定 NUMA node，避免跨节点访问 |
| DMA 映射开销 | page_pool 缓存 DMA 映射，避免重复 map/unmap |

## 与 Rosen 3.x 的差异

| 维度 | Rosen（3.x） | 现代（5.x/6.x） |
|------|-------------|----------------|
| Rx buffer 分配 | alloc_page 每次分配 | page_pool recycling |
| DMA 映射 | 每次收包 map/unmap | 缓存映射 |
| XDP 支持 | 不存在 | page_pool 是 XDP 基础设施 |
