# Ch 11 §3 分配交换槽 (Allocating Swap Slots)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

### 3. 分配交换槽 (Allocating Swap Slots)

在 swap 区内找 **空闲的一页大小 slot**。

### 簇 (Cluster · `SWAPFILE_CLUSTER`)

| 动机 | 做法 |
|------|------|
| 磁盘 **随机 seek 慢** | 尽量 **连续分配** 多个 slot（一 **cluster**） |
| 假设 | **同时换出的页** 很可能 **一起换入** — **相邻磁盘块** → **顺序 I/O** |

**HFT：** 若不幸发生 swap，**cluster** 只减轻 **磁盘** 侧 — 仍 **远慢于 RAM**。

---
