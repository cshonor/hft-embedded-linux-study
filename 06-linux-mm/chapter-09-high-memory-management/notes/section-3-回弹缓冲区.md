# Ch 9 §3 回弹缓冲区 (Bounce Buffers)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

### 3. 回弹缓冲区 (Bounce Buffers)

**场景：** 设备 **DMA 只能寻址低端物理地址**（32 位设备接 64 位机、PAE、部分旧控制器）— **无法** 直接读写 **HIGHMEM 物理页**。

```
设备 DMA 写 ──► 低端 bounce buffer（ZONE_DMA / 设备可见区）
                    │
                    ▼ 内核 memcpy
              HIGHMEM 目标 struct page
```

| 方向 | 流程 |
|------|------|
| **设备 → 内存（读盘/网卡入）** | 数据先进 **bounce** → **复制** 到高端目标页 |
| **内存 → 设备（写）** | 从高端页 **复制到 bounce** → 设备 DMA |

**代价：** **多一次完整拷贝** — 仍可能比 **为腾 LOWMEM 而 swap 整进程** 便宜。

**HFT 现代对照：** **NIC DMA 到 registered 物理地址** — 必须 **在设备 `dma_mask` 内** 分配缓冲（**ibverbs / DPDK memzone**）；不是 HIGHMEM，但是 **同一「硬件看不见高物理地址」** 问题。

---
