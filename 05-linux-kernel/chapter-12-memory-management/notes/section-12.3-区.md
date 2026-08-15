## ③ 区 · Zones

并非所有 **物理页** 对内核都 **同等可访问** — **区（zone）** 按 **地址能力** 与 **用途** 划分 **伙伴系统 free list**。

#### 四类区（x86 典型）

| 区 | 用途 |
|----|------|
| **ZONE_DMA** | **ISA 时代** DMA 只能寻址 **低 16MB** 物理 — 老设备 |
| **ZONE_DMA32** | **32 位 DMA 掩码** 设备 — 可访问 **< 4GB** 物理 |
| **ZONE_NORMAL** | 可 **永久映射** 进内核 **线性地址空间** 的页 |
| **ZONE_HIGHMEM** | **物理存在** 但 **无永久内核线性映射**（典型：**x86 32 位** 896MB 以上） |

```
物理地址升高 ──────────────────────────────────────►

  ZONE_DMA   │  ZONE_DMA32  │     ZONE_NORMAL      │  ZONE_HIGHMEM
  (低地址)   │              │  (direct map 可达)    │  (需 kmap 临时)
             │              │                      │
         老 DMA 设备      现代 PCI DMA          内核 kmalloc 默认来源
```

#### 为何需要分区

| 约束 | 后果 |
|------|------|
| **DMA 寻址宽度** | 设备只能看 **低物理地址** — 须从 **ZONE_DMA*** 留页 |
| **32 位内核 VA 有限** | 内核 **直接映射窗口** 固定 — 超出部分 = **HIGHMEM** |
| **水位（watermark）** | 每 zone **min/low/high** — 低于阈值 **直接回收 / kswapd**（Ch 16 方向） |

#### 64 位与嵌入式

| 平台 | HIGHMEM |
|------|---------|
| **x86-64 / arm64 服务器** | 通常 **无 ZONE_HIGHMEM** — 全物理可 direct map |
| **ARM32 3:1 分割** | 可能有 **HIGHMEM** — 驱动访问高端页用 **`kmap_atomic`** |
| **SoC 小 RAM** | 区仍分 **DMA32/NORMAL** — **CMA** 常占 **NORMAL** 一段 |

#### `gfp_zone()` 与分配回落

| 行为 | 说明 |
|------|------|
| **`GFP_KERNEL`** | 优先 **NORMAL**；可 **回落** 到其他 zone（视标志） |
| **`GFP_DMA` / `GFP_DMA32`** | **强制** 从对应 zone 取 — 失败则 NULL |
| **`__GFP_HIGHMEM`** | 允许分配 **不可直接映射** 的页 — 访问前 **kmap** |

**HFT：** 用户态 **DMA-BUF / RDMA** 注册内存时，驱动在内核 **`dma_alloc_coherent`** 从 **合适 zone** 拿 **物理连续 + 设备可见** 的页 — 懂 zone 才懂 **`swiotlb` bounce buffer**（物理页不在设备掩码内时 **拷贝**）。

→ [06 Gorman Ch2 内存区域](../../../../06-linux-mm/chapter-02-describing-physical-memory/notes/section-2-内存区域.md) · [Ch 12.9 HIGHMEM](./section-12.9-高端内存的映射.md)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** ZONE_DMA、ZONE_NORMAL、ZONE_HIGHMEM 分别是什么？x86_64 还有 HIGHMEM 吗？

<details><summary>答案</summary>

ZONE_DMA：< 16MB（ISA DMA 限制）；ZONE_NORMAL：16MB-896MB（直接映射）；ZONE_HIGHMEM：> 896MB（需 kmap 临时映射）。x86_64 没有 HIGHMEM——64 位地址空间足够直接映射所有物理内存。HIGHMEM 是 32 位内核的历史包袱。

</details>

**Q2.** 为什么 GFP_KERNEL 可能从 ZONE_NORMAL 分配而 GFP_DMA 从 ZONE_DMA？

<details><summary>答案</summary>

GFP_DMA 保证物理地址 < 16MB（ISA 设备只能寻址 24 位地址）。GFP_KERNEL 无地址限制，优先从 ZONE_NORMAL 分配（直接映射，速度快）。如果 ZONE_NORMAL 不足，buddy 系统会从 ZONE_DMA 挪页（fallback），但会保留 DMA 备用页防止饿死。

</details>

</details>
---
