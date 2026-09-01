# Madieu 第 12 章 — DMA - Direct Memory Access

> 对应目录：`chapter-12-dma/`  
> 书：*Linux Device Drivers Development* — John Madieu（内核约 4.1–4.13）  
> 大纲：[../OUTLINE.md](../OUTLINE.md) · 评测：[../../MADIEU-EVAL.md](../../MADIEU-EVAL.md)

**优先级**：12.1 / 12.2 **精读（低延迟）**，12.3–12.5 精读  
**板卡**：树莓派 ARM64（5.x+ 代码需少量适配）

---

## 章节核心定位

DMA = **硬件自己搬运内存数据，CPU 不亲自拷贝**。本章是低延迟路线的核心章之一：网卡收包路径上每一次"CPU 亲自搬运"都是延迟与 CPU 周期的双重浪费；DPDK 的性能基石正是 DMA + 内核旁路。

一句话浓缩（详解见扩展精读）：老方式——中断 CPU，CPU 亲自访存拷贝到内存；DMA 方式——配置完，硬件自己拷贝到内存，CPU 解放，结束才中断。**结局都到内存，干活的角色换了。**

---

## 小节状态

| 节 | 主题 | 标签 | 状态 |
|----|------|------|------|
| 前置 | DMA 基础（搬运机制 / 地址空间 / 两种模式） | **精读** | ✅ [12.0 扩展精读](./12.0-dma-fundamentals.md) |
| 12.1 | 缓存一致性 | **精读（低延迟）** | ✅ [12.1 扩展精读](./12.1-cache-coherence.md) |
| 12.2 | 一致性 / 流式映射 | **精读** | ✅ [12.2 扩展精读](./12.2-coherent-streaming-mappings.md) |
| 12.3 | scatter/gather | 精读 | 待填 |
| 12.4 | DMA Engine | 精读 | 待填 |
| 12.5 | DTS 绑定 DMA | 精读 | 待填 |

---

## 扩展精读

**[12.0-dma-fundamentals.md](./12.0-dma-fundamentals.md)** — DMA 基础：

- 无 / 有 DMA 全流程（ASCII 图，下沉到指令层：PIO `in/out`、MMIO `load/store`）
- ⭐ "终点都是内存"洞察与误区澄清（不走 CPU 寄存器流水线 ≠ 不走内存）
- DMA 地址 ≠ 虚拟地址（`dma_handle` 是总线地址；IOMMU 平台上 = IOVA ≠ 物理地址，按 v6.6 `dma-api.rst` 核对修正）
- coherent vs streaming 两模式对比、`copy_from_user` vs DMA 对比
- HFT / 嵌入式关联、`dma_alloc_coherent` 描述符环代码骨架、自测 4 问

**[12.1-cache-coherence.md](./12.1-cache-coherence.md)** — 缓存一致性：

- 两个方向的 stale 数据 ASCII 图 + cacheline 共享损坏（引 howto 原文）
- x86 vs ARM 平台对比（snoop 硬件一致 vs 软件维护）
- ⭐ x86 sync 空操作**双保险证据链**（v6.6：空 static inline 桩 + `dev_is_dma_coherent()` 恒 true 双重机制）
- `dma_sync_*` 方向语义表（`for_X` = 接下来归 X 用）、两条解决路线、自测 3 问

**[12.2-coherent-streaming-mappings.md](./12.2-coherent-streaming-mappings.md)** — 一致性 / 流式映射 API：

- 两类映射官方画像 + API 全景表（按场景选：大块/小块/一次传输/循环复用）
- `dma_pool` 系列（align 幂二/alloc 边界约束）、`dma_map_single` 两条硬约束（物理连续 + dma_mask）
- 方向枚举表与 BIDIRECTIONAL 双 sync 规则、`dma_mapping_error` 错误检查（不能只判 0）
- ⭐ `dma_need_sync()` 官方接口（v6.6 实现链：mapping.c:822 → direct.c:610，x86 几乎恒 false，唯一例外 swiotlb）
- 收包路径完整生命周期伪代码（howto 精简）+ coherent 映射仍需 wmb() 的顺序性陷阱、自测 3 问

---

## 与本仓库的咬合

| 方向 | 链接 |
|------|------|
| 上一章 | [Ch11 内核内存管理](../chapter-11-kernel-memory-management/notes.md)（`kmalloc` 虚拟地址为什么不能给 DMA） |
| 思想源头 | [LDD3 Ch15 Memory Mapping and DMA](../../classic-driver-theory/chapter-15-memory-mapping-and-dma/notes.md)（2.6.10 老 API，思想精读） |
| 中断侧 | [LDD3 Ch10 Interrupt Handling](../../classic-driver-theory/chapter-10-interrupt-handling/notes.md)（DMA 完成中断的上/下半部） |
| `copy_from_user` | [Ch4 字符设备驱动](../chapter-04-character-device-drivers/notes.md) |
| HFT 落地 | [13-dpdk](../../../13-dpdk/01-Intro-Book/)（DMA + 内核旁路的工程化） |

---

## 参考

- Madieu, *Linux Device Drivers Development*, Chapter 12
- Linux v6.6 `Documentation/core-api/dma-api.rst`（API 签名核对来源，详见 12.0 扩展精读）
