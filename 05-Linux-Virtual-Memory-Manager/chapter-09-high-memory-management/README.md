# Ch 9 高端内存管理 · High Memory Management

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

---

## 本章概述

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过**（**x86_64 HFT 机器通常无 `ZONE_HIGHMEM`** — 读作 **32 位 / PAE 历史** + **DMA 寻址限制** 背景；**`mempool` 概念** 见 [Ch 8](../../chapter-08-slab-allocator/)）

## 问题从哪来

**32 位 x86** 典型划分：**3GiB 用户 + 1GiB 内核**（`PAGE_OFFSET = 0xC0000000`）。

| 区域 | 大致范围 | 内核能否 **直接** 用虚拟地址访问 |
|------|----------|----------------------------------|
| **`ZONE_NORMAL`** | 低端物理 RAM（原书约 **896MiB** 直接映射窗口） | **能** — `__va(pfn)` 线性映射 |
| **`ZONE_HIGHMEM`** | 超出直接映射窗口的物理 RAM | **不能** — 必须 **临时映射** |

物理内存 **> ~1GiB**（PAE 下可达 **64GiB**）时，大量页框落在 **HIGHMEM** — 内核 **不能** 像 touch `ZONE_NORMAL` 那样 **直接 dereference** 对应物理页。

→ 铺垫：[Ch 2 §4 高端内存](../../chapter-02-describing-physical-memory/notes/section-4-高端内存.md#4-高端内存-high-memory) · [Ch 3 §5 物理↔虚拟](../../chapter-03-page-table-management/notes/section-5-地址与-struct-page-的映射.md#5-地址与-struct-page-的映射)

**x86_64：** 内核 **64 位 canonical 地址 + 巨大直接映射**，**HIGHMEM 通常为空** — 本章机制 **多数不跑**；但 **bounce buffer / 设备 DMA 掩码** 在 **「设备只能看低 4G 物理地址」** 时仍相关。

---

## 本章在 VM 子系统中的位置

```
Ch 2 ZONE_HIGHMEM 页框存在，但内核不能常触
        ↓
Ch 9 kmap / bounce / emergency pool  ← 本章
        ↓
Ch 10 回收、I/O 路径可能 touch HIGHMEM 页
Ch 8 mempool — 2.6 从 emergency pool 泛化而来
```

**HFT（64 位）：** 不必背 PKMap 细节；理解 **「不能假设所有 RAM 都能零成本 kernel 直接访问」** 与 **DMA 缓冲在「设备可见物理区」** 即可 — 对照 **DPDK 在 DMA 区分配 mbuf**。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. PKMap 地址空间管理 | [notes/section-1-PKMap-地址空间管理.md](./notes/section-1-PKMap-地址空间管理.md) |
| 2. 映射与解除映射高端页 | [notes/section-2-映射与解除映射高端页.md](./notes/section-2-映射与解除映射高端页.md) |
| 3. 回弹缓冲区 | [notes/section-3-回弹缓冲区.md](./notes/section-3-回弹缓冲区.md) |
| 4. 紧急内存池 | [notes/section-4-紧急内存池.md](./notes/section-4-紧急内存池.md) |
| 5. 2.6 内核的新变化 | [notes/section-5-2.6-内核的新变化.md](./notes/section-5-2.6-内核的新变化.md) |

---

## 相关章节

- 上一章：[../chapter-08-slab-allocator/](../chapter-08-slab-allocator/)
- 下一章：[../chapter-10-page-frame-reclamation/](../chapter-10-page-frame-reclamation/)
- 附录 I：[../../appendix-I-高端内存管理.md](../../appendix-I-高端内存管理.md)
- 全书目录：[OUTLINE.md](../../OUTLINE.md)
