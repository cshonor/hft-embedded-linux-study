# §16.3 DMA 一致性

> **来源：** [Ch16 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

DMA 直接到内存不经过 CPU cache，导致数据不一致。DMA 写内存前 invalidate CPU cache，DMA 读内存前 clean CPU cache。自修改代码需要 clean D-cache + invalidate I-cache。

## 核心要点

### 三种场景

| 场景 | 问题 | 解决 |
|------|------|------|
| DMA→内存 | CPU Cache 有旧数据 | 先 **invalidate** |
| 内存→DMA | CPU Cache 有新数据未写回 | 先 **clean** |
| 自修改代码 | I-Cache 有旧指令 | 先 clean D-cache → invalidate I-cache |

### Linux DMA API

```c
// 设备→内存：invalidate
dma_map_single(dev, addr, size, DMA_FROM_DEVICE);

// 内存→设备：clean
dma_map_single(dev, addr, size, DMA_TO_DEVICE);

// 完成后恢复
dma_unmap_single(dev, addr, size, direction);
```

> 如果硬件支持 **IOMMU/SMMU**，可能自动维护一致性，不需要软件 flush。

### Coherent vs Non-coherent DMA

| 类型 | 说明 | 需要 flush？ |
|------|------|-------------|
| Coherent DMA | 硬件自动维护 cache 一致性 | **不需要** |
| Non-coherent DMA | 硬件不维护 | **需要**软件 flush |

> Pi4B/Pi5 的 DMA 通常是 non-coherent，需要软件 flush。

## HFT 关联

HFT 系统如果用 DMA 收发网络包（如 DPDK），必须正确处理 cache 一致性。`DMA_FROM_DEVICE`（invalidate）用于接收，`DMA_TO_DEVICE`（clean）用于发送。在 Pi5 上做裸金属 HFT，没有 Linux DMA API，需要手动 `dc ivac`/`dc cvac`。如果 HFT 平台支持 coherent DMA（如某些服务器 SoC），可以省去 flush 开销，显著降低延迟。

## 自测题

1. **DMA 从设备读数据到内存，CPU 应该先做什么？为什么？**

<details>
<summary>答案</summary>

先 **invalidate** CPU cache 对应区域。因为 DMA 写入新数据到内存，但 CPU cache 可能缓存了旧数据。invalidate 后 CPU 读时 cache miss → 从内存读到 DMA 写入的新值。如果不 invalidate，CPU 可能读到 cache 中的旧值。
</details>

2. **Coherent DMA 和 Non-coherent DMA 的区别？Pi5 属于哪种？**

<details>
<summary>答案</summary>

- **Coherent**：硬件（IOMMU/SMMU）自动维护 CPU cache 和 DMA 的一致性，**不需要软件 flush**
- **Non-coherent**：硬件不维护，**需要软件手动** invalidate/clean

Pi4B/Pi5 的 DMA 通常是 **non-coherent**，需要软件 flush。
</details>

3. **Linux 的 `dma_map_single(dev, addr, size, DMA_TO_DEVICE)` 内部做了什么？**

<details>
<summary>答案</summary>

对 `[addr, addr+size)` 区域执行 **clean cache**（`dc cvac`）+ DSB。将 CPU cache 中的脏数据写回内存，确保 DMA 能从内存读到最新数据。等价于"内存→设备"方向的 cache 维护。`DMA_FROM_DEVICE` 则做 invalidate，`DMA_BIDIRECTIONAL` 做 clean + invalidate。
</details>

## 参考与延伸

- [§16.1 MESI 协议](01-mesi.md) — 多核 cache 一致性
- [§16.4 自修改代码](04-self-modifying-code.md) — I-Cache 一致性
- [Ch15 §15.5 DMA 与 Cache](../../chapter-15-cache-basics/notes/section-0-本章完整概述.md) — DMA cache 操作基础
