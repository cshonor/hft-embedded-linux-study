# §15.5 DMA 与 Cache

> **来源：** [Ch15 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

DMA 直接到内存不经过 CPU Cache，导致数据不一致。DMA 写内存前需 invalidate CPU Cache，DMA 读内存前需 clean CPU Cache。这是嵌入式驱动的必考点。

## 核心要点

### 两种 DMA 场景

```
场景1：DMA 写内存（设备→内存）
  CPU Cache 有旧数据 → CPU 读到旧值
  解决：DMA 前先 invalidate CPU Cache 对应区域

场景2：DMA 读内存（内存→设备）
  CPU 写了新数据在 Cache 中，还没写回 → DMA 读到旧值
  解决：DMA 前先 clean（flush）CPU Cache 对应区域
```

### 操作总结

| DMA 方向 | CPU 需要做的 | 原因 |
|----------|-------------|------|
| 设备 → 内存（DMA 写） | **Invalidate** CPU cache | 丢弃旧数据，让 CPU 之后从内存读新值 |
| 内存 → 设备（DMA 读） | **Clean** CPU cache | 写回新数据到内存，让 DMA 能读到 |

### Linux DMA API

```c
// 设备→内存：invalidate
dma_map_single(dev, addr, size, DMA_FROM_DEVICE);

// 内存→设备：clean
dma_map_single(dev, addr, size, DMA_TO_DEVICE);

// 双向：clean + invalidate
dma_map_single(dev, addr, size, DMA_BIDIRECTIONAL);

// 完成后恢复
dma_unmap_single(dev, addr, size, direction);
```

> 如果硬件支持 **IOMMU/SMMU**，可能自动维护一致性，不需要软件 flush。

## HFT 关联

HFT 系统如果用 DMA 接收网卡数据（如 DPDK），必须正确处理 cache 一致性。DPDK 通常用 `DMA_FROM_DEVICE` 映射（invalidate 后 DMA 写入，CPU 读取新数据）。如果忘记 invalidate，CPU 会读到 cache 中的旧数据——这是 DMA 驱动中最常见的 bug。在 Pi5 上，如果 SMMU 配置正确，可能自动维护一致性，但裸金属 HFT 通常没有 SMMU，必须手动处理。

## 自测题

1. **DMA 从设备写数据到内存后，为什么 CPU 可能读到旧值？怎么修复？**

<details>
<summary>答案</summary>

原因：CPU cache 中缓存了该内存区域的旧数据，DMA 直接写内存（不更新 cache），CPU 读时命中 cache 读到旧值。修复：DMA 写之前 **invalidate** CPU cache 对应区域，这样 CPU 读时 cache miss → 从内存读到 DMA 写入的新数据。
</details>

2. **CPU 准备数据给 DMA 读取前，为什么要 clean cache？**

<details>
<summary>答案</summary>

CPU 写数据时可能只更新了 cache（Write-Back 模式），新数据还没写回内存。DMA 直接从内存读 → 读到旧值。Clean cache 强制将脏数据写回内存，确保 DMA 能读到最新数据。
</details>

3. **如果硬件支持 IOMMU/SMMU，还需要软件做 cache flush 吗？**

<details>
<summary>答案</summary>

**可能不需要**（取决于 IOMMU 配置）。IOMMU/SMMU 可以自动维护 CPU cache 和 DMA 之间的一致性（coherent DMA）。但裸金属系统通常没有 IOMMU，或者 IOMMU 配置为非 coherent 模式，仍需手动 flush。需要查平台文档确认是否支持 coherent DMA。
</details>

## 参考与延伸

- [§15.4 关键概念](04-key-concepts.md) — Clean/Invalidate/Flush 定义
- [§15.7 易错点](07-pitfalls.md) — DMA cache 操作的常见错误
- [Ch16 §16.3 DMA 一致性](../../chapter-16-cache-coherency/notes/section-0-本章完整概述.md) — DMA 一致性的完整分析
