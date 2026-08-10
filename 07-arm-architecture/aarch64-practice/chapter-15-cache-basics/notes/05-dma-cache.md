# §15.5 DMA 与 Cache

> **来源：** [Ch15 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

DMA 直接到内存不经过 CPU Cache，导致数据不一致。DMA 写内存前需 invalidate CPU Cache，DMA 读内存前需 clean CPU Cache。这是嵌入式驱动的必考点，也是 HFT 网络栈的核心知识。

## 核心要点

### 两种 DMA 场景

```
场景1：DMA 写内存（设备→内存）
  1. CPU cache 有旧数据
  2. DMA 直接写内存（不更新 cache）
  3. CPU 读时命中 cache → 读到旧值！
  解决：DMA 前先 invalidate CPU cache 对应区域

场景2：DMA 读内存（内存→设备）
  1. CPU 写了新数据在 cache 中（Write-Back），还没写回
  2. DMA 直接从内存读 → 读到旧值！
  解决：DMA 前先 clean（写回）CPU cache 对应区域
```

### 操作总结

| DMA 方向 | CPU 需要做的 | 指令 | 原因 |
|----------|-------------|------|------|
| 设备 → 内存（DMA 写） | **Invalidate** | `dc ivac` | 丢弃旧数据，让 CPU 之后从内存读新值 |
| 内存 → 设备（DMA 读） | **Clean** | `dc cvac` | 写回新数据到内存，让 DMA 能读到 |
| 双向 | **Clean + Invalidate** | `dc civac` | 两种操作都做 |

### DMA cache 操作流程图

```
┌─────────────────────────────────────────────────────────┐
│                   DMA 接收流程（设备→内存）                │
├─────────────────────────────────────────────────────────┤
│ 1. CPU 准备接收 buffer                                    │
│ 2. dc ivac, buffer_addr  ← invalidate CPU cache         │
│ 3. dsb sy                ← 等待 invalidate 完成          │
│ 4. 启动 DMA 传输（设备写内存）                            │
│ 5. 等待 DMA 完成（中断或轮询）                            │
│ 6. CPU 读取 buffer → cache miss → 从内存读 DMA 写入数据  │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│                   DMA 发送流程（内存→设备）                │
├─────────────────────────────────────────────────────────┤
│ 1. CPU 在 buffer 中写入数据                              │
│ 2. dc cvac, buffer_addr  ← clean（写回脏数据到内存）     │
│ 3. dsb sy                 ← 等待 clean 完成             │
│ 4. 启动 DMA 传输（设备从内存读）                          │
│ 5. 等待 DMA 完成                                         │
└─────────────────────────────────────────────────────────┘
```

### Linux DMA API

```c
// 设备→内存：invalidate
dma_addr_t dma_handle = dma_map_single(dev, addr, size, DMA_FROM_DEVICE);
// 内部执行: dc ivac + dsb sy

// 内存→设备：clean
dma_addr_t dma_handle = dma_map_single(dev, addr, size, DMA_TO_DEVICE);
// 内部执行: dc cvac + dsb sy

// 双向：clean + invalidate
dma_addr_t dma_handle = dma_map_single(dev, addr, size, DMA_BIDIRECTIONAL);
// 内部执行: dc civac + dsb sy

// 完成后恢复（unmap 时自动做对应操作）
dma_unmap_single(dev, dma_handle, size, direction);
```

### 裸金属 DMA cache 操作

```c
// 裸金属代码中的 DMA cache 操作
void dma_cache_invalidate(void *addr, size_t size) {
    uint64_t va = (uint64_t)addr;
    uint64_t end = va + size;
    // 按 cache line 对齐
    va &= ~0x3FULL;
    while (va < end) {
        asm volatile("dc ivac, %0" :: "r"(va));
        va += 64;  // cache line = 64 字节
    }
    asm volatile("dsb sy");
}

void dma_cache_clean(void *addr, size_t size) {
    uint64_t va = (uint64_t)addr;
    uint64_t end = va + size;
    va &= ~0x3FULL;
    while (va < end) {
        asm volatile("dc cvac, %0" :: "r"(va));
        va += 64;
    }
    asm volatile("dsb sy");
}
```

> 如果硬件支持 **IOMMU/SMMU**，可能自动维护一致性，不需要软件 flush。

### Coherent vs Non-coherent DMA

| 类型 | 说明 | 需要 flush？ | 性能 |
|------|------|-------------|------|
| Coherent DMA | 硬件自动维护 cache 一致性 | **不需要** | 好（省 flush 开销） |
| Non-coherent DMA | 硬件不维护 | **需要**软件 flush | 差（每次 flush ~50-100ns） |

> Pi4B/Pi5 的 DMA 通常是 non-coherent，需要软件 flush。
> 某些服务器 SoC（如 Ampere Altra）支持 coherent DMA。

## HFT 关联

HFT 系统如果用 DMA 接收网卡数据（如 DPDK），必须正确处理 cache 一致性。DPDK 通常用 `DMA_FROM_DEVICE` 映射（invalidate 后 DMA 写入，CPU 读取新数据）。如果忘记 invalidate，CPU 会读到 cache 中的旧数据——这是 DMA 驱动中最常见的 bug。在 Pi5 上，如果 SMMU 配置正确，可能自动维护一致性，但裸金属 HFT 通常没有 SMMU，必须手动处理。

每次 DMA cache flush 约 50-100ns（64 字节 cache line），对 HFT 延迟有显著影响。使用大 buffer 减少 flush 频率。

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

4. **DMA 接收时先 invalidate 再 DMA 传输，如果 DMA 传输期间 CPU 又写了 cache 会怎样？**

<details>
<summary>答案</summary>

invalidate 后到 DMA 完成期间，如果 CPU 写了该区域的 cache，cache 会重新加载数据（因为之前 invalidate 了，写操作会 miss → 从内存加载 → 修改 → cache 中有新脏数据）。DMA 同时也在写内存 → 竞争。DMA 完成后 CPU cache 中有 DMA 之前写的旧内存 + CPU 自己写的新数据 → 数据不一致。

修复：DMA 传输期间 CPU 不应访问该 buffer（这是 DMA buffer 的基本规则）。正确流程：invalidate → DMA 传输（CPU 不碰 buffer）→ DMA 完成 → CPU 读取。
</details>

## 参考与延伸

- [§15.4 关键概念](04-key-concepts.md) — Clean/Invalidate/Flush 定义
- [§15.7 易错点](07-pitfalls.md) — DMA cache 操作的常见错误
- [Ch16 §16.3 DMA 一致性](../../chapter-16-cache-coherency/notes/03-dma-coherency.md) — DMA 一致性的完整分析
