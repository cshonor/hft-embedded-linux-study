# §16.3 DMA 一致性

> **来源：** [Ch16 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

DMA 直接访问内存不经过 CPU cache，导致数据不一致。本节分析 DMA 与 CPU cache 的三种交互场景、Linux DMA API 的使用、coherent vs non-coherent DMA 的区别，以及在裸金属系统中如何手动维护 cache 一致性。

## 核心要点

### 三种场景

| 场景 | 问题 | 解决 | 方向 |
|------|------|------|------|
| DMA→内存 | CPU Cache 有旧数据，DMA 写新数据到内存 | 先 **invalidate** CPU cache | DMA_FROM_DEVICE |
| 内存→DMA | CPU Cache 有新数据未写回，DMA 读内存读旧值 | 先 **clean** CPU cache | DMA_TO_DEVICE |
| 自修改代码 | I-Cache 有旧指令，D-cache 有新指令未写回 | clean D-cache → invalidate I-cache | 双向 |

### DMA 接收流程（DMA→内存）

```
1. CPU 分配接收缓冲区 rx_buf
2. CPU 执行 dc ivac（invalidate rx_buf 区域的 cache）
3. CPU 配置 DMA 控制器（源=网卡 FIFO，目的=rx_buf 物理地址）
4. DMA 启动传输，直接写内存（绕过 CPU cache）
5. DMA 完成，CPU 收到中断
6. CPU 读 rx_buf → cache miss → 从内存读 → 得到 DMA 写入的新数据
```

### DMA 发送流程（内存→DMA）

```
1. CPU 填充 tx_buf（数据在 D-cache 中，内存可能不是最新）
2. CPU 执行 dc cvac（clean tx_buf 区域的 cache，写回内存）
3. CPU 配置 DMA 控制器（源=tx_buf 物理地址，目的=网卡 FIFO）
4. DMA 启动传输，从内存读数据（此时内存是最新）
5. DMA 完成发送
```

### 裸金属 DMA Cache 操作

```c
#define CACHE_LINE_SIZE 64

// DMA 接收前：invalidate CPU cache
void dma_cache_invalidate(void *addr, size_t size) {
    uintptr_t start = (uintptr_t)addr;
    uintptr_t end = start + size;
    // 对齐到 cache line 边界
    start &= ~(CACHE_LINE_SIZE - 1);
    
    asm volatile(
        "1: dc ivac, %0\n"        // invalidate to PoC
        "   add %0, %0, #64\n"
        "   cmp %0, %1\n"
        "   b.lo 1b\n"
        "   dsb sy\n"             // 确保所有 invalidate 完成
        : : "r"(start), "r"(end) : "memory"
    );
}

// DMA 发送前：clean CPU cache
void dma_cache_clean(void *addr, size_t size) {
    uintptr_t start = (uintptr_t)addr;
    uintptr_t end = start + size;
    start &= ~(CACHE_LINE_SIZE - 1);
    
    asm volatile(
        "1: dc cvac, %0\n"        // clean to PoC
        "   add %0, %0, #64\n"
        "   cmp %0, %1\n"
        "   b.lo 1b\n"
        "   dsb sy\n"             // 确保所有 clean 完成
        : : "r"(start), "r"(end) : "memory"
    );
}

// 使用示例
void dma_receive(void *buf, size_t len) {
    dma_cache_invalidate(buf, len);   // 1. invalidate
    dma_start_rx(buf_phys, len);     // 2. 启动 DMA
    dma_wait_complete();             // 3. 等待完成
    // 此时 CPU 读 buf 得到新数据
}

void dma_send(void *buf, size_t len) {
    dma_cache_clean(buf, len);       // 1. clean
    dma_start_tx(buf_phys, len);    // 2. 启动 DMA
    dma_wait_complete();             // 3. 等待完成
}
```

### Linux DMA API

```c
// 设备→内存：invalidate
dma_addr_t dma_handle = dma_map_single(dev, addr, size, DMA_FROM_DEVICE);
// 内部：dc ivac + dsb

// 内存→设备：clean
dma_addr_t dma_handle = dma_map_single(dev, addr, size, DMA_TO_DEVICE);
// 内部：dc cvac + dsb

// 双向：clean + invalidate
dma_addr_t dma_handle = dma_map_single(dev, addr, size, DMA_BIDIRECTIONAL);

// 完成后取消映射
dma_unmap_single(dev, dma_handle, size, direction);
```

> 如果硬件支持 **IOMMU/SMMU**，可能自动维护一致性，不需要软件 flush。

### Coherent vs Non-coherent DMA

| 类型 | 说明 | 需要 flush？ | 典型平台 |
|------|------|-------------|----------|
| Coherent DMA | 硬件自动维护 cache 一致性 | **不需要** | 服务器 SoC（有 IOCCI） |
| Non-coherent DMA | 硬件不维护 | **需要**软件 flush | Pi4B/Pi5 |

### DMA 方向与 cache 操作对应表

| DMA 方向 | 数据流向 | cache 操作 | ARMv8 指令 |
|----------|---------|-----------|-----------|
| DMA_FROM_DEVICE | 设备→内存 | invalidate | `dc ivac` |
| DMA_TO_DEVICE | 内存→设备 | clean | `dc cvac` |
| DMA_BIDIRECTIONAL | 双向 | clean + invalidate | `dc civac` |

> Pi4B/Pi5 的 DMA 通常是 non-coherent，需要软件 flush。

### Common 同步 API

```c
// dma_sync_* 用于已映射后再次同步
dma_sync_single_for_cpu(dev, dma_handle, size, DMA_FROM_DEVICE);
// 内部：invalidate（CPU 要读 DMA 写入的数据）

dma_sync_single_for_device(dev, dma_handle, size, DMA_TO_DEVICE);
// 内部：clean（CPU 写了新数据，DMA 要读）
```

## HFT 关联

HFT 系统如果用 DMA 收发网络包（如 DPDK），必须正确处理 cache 一致性。`DMA_FROM_DEVICE`（invalidate）用于接收，`DMA_TO_DEVICE`（clean）用于发送。在 Pi5 上做裸金属 HFT，没有 Linux DMA API，需要手动 `dc ivac`/`dc cvac`。

### DPDK 中的 DMA cache 管理

```c
// DPDK rte_mbuf 的 DMA 映射
rte_mbuf *mbuf = rte_pktmbuf_alloc(mp);
// 接收前：rte_pktmbuf 数据区 invalidate
rte_pktmbuf_reset(mbuf);  // 内部含 cache invalidate

// 发送前：clean
rte_ioat_enqueue_copy(dev_id, src_phys, dst_phys, len, ...);
// 内部含 cache clean
```

如果 HFT 平台支持 coherent DMA（如某些服务器 SoC 有 IOCCI），可以省去 flush 开销，显著降低延迟。检测方法：检查 `CTR_EL0.DIC` 和 `CTR_EL0.IDC` 位。

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

4. **DMA 接收数据后直接读 buffer，没有 invalidate，会发生什么？**

<details>
<summary>答案</summary>

如果 buffer 之前被 CPU 访问过，cache 中可能缓存了旧数据。CPU 读时 cache hit → 读到**旧值**（DMA 写入内存的新值被忽略）。这种 bug 非常隐蔽——开发机上 cache miss 概率不同，可能"偶尔"读到正确值，测试难以复现。必须始终在 DMA 接收前 invalidate。
</details>

## 参考与延伸

- [§16.1 MESI 协议](01-mesi.md) — 多核 cache 一致性
- [§16.4 自修改代码](04-self-modifying-code.md) — I-Cache 一致性
- [Ch15 §15.5 DMA 与 Cache](../../chapter-15-cache-basics/notes/section-0-本章完整概述.md) — DMA cache 操作基础
