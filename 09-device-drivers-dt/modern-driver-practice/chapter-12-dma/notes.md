# Madieu 第 12 章 — DMA - Direct Memory Access

> 对应目录：`chapter-12-dma/`  
> 书：*Linux Device Drivers Development* — John Madieu（内核约 4.1–4.13）  
> 大纲：[../OUTLINE.md](../OUTLINE.md) · 评测：[../../MADIEU-EVAL.md](../../MADIEU-EVAL.md)

**优先级**：12.1 / 12.2 **精读（低延迟）**，12.3–12.5 精读  
**板卡**：树莓派 ARM64（5.x+ 代码需少量适配）  
**本篇状态**：开篇总览 + DMA 基础（对应 12.1/12.2 前置概念）已填；12.3 scatter/gather、12.4 DMA Engine、12.5 DTS 绑定待填。

---

## 本节讲什么

**DMA = Direct Memory Access 直接内存访问：硬件自己搬运内存数据，不需要 CPU 亲自拷贝。**

比喻：没有 DMA 时老板（CPU）亲自一箱一箱搬货；有 DMA 时雇搬运工（DMA 控制器）搬货——老板只需下达指令"把这块数据从 A 地址搬到 B 地址"，然后回去办公；搬运工干完活，敲门（**中断**）通知老板完工。

---

## 要点

### 没有 DMA 会怎样（以网卡收包为例）

```
【无 DMA】                                【有 DMA】
┌──────┐  每小段数据一次中断              ┌──────┐
│ 网卡 │──┐                              │ 网卡 │──┐
└──────┘  │ 硬件小缓冲区                  └──────┘  │
          ▼                                       ▼
     ┌─────────┐                            ┌──────────┐
     │ CPU 亲拷 │ 一字一字搬到内存           │ DMA 控制器│ 自主搬运到内存
     └─────────┘                            └──────────┘
          │                                       │ 搬完才发 1 次中断
          ▼                                       ▼
   CPU 全程被占用，                            CPU 并行跑别的任务，
   干不了别的事                               只需处理包本身
```

| | 无 DMA（老式 PIO） | 有 DMA |
|---|---|---|
| 谁搬运数据 | **CPU 亲自拷** | **DMA 硬件搬运** |
| 中断次数 | 每来一小块数据中断一次 | **整块搬完才 1 次中断** |
| CPU 占用 | 全程占用，大量时间耗在搬运苦力活 | 只下达命令 + 收完工通知 |

### 两个典型场景

| 场景 | 流程 | 备注 |
|------|------|------|
| **网卡 DMA** | 网卡收包 → DMA 直接写入内存缓冲区 → 完成后中断，CPU 只处理包不拷贝 | **DPDK 高性能网络重度依赖**；HFT 低延迟基石 |
| **SD 卡 / USB 麦克风** | 麦克风持续采集音频样本 → DMA 持续搬进内存，整块搬完才 1 次中断 | 而不是每个采样点中断一次 |

### ⚠️ 关键概念：DMA 地址 ≠ 虚拟地址

- **DMA 硬件大多不理解虚拟地址**——设备发起的总线事务**不经过 CPU 的 MMU**。用户态/内核态用的都是虚拟地址，DMA 硬件访问内存必须给它**总线侧的地址**。
- 内核 API：`dma_alloc_coherent()` 专门分配物理连续内存，**返回值是 CPU 侧虚拟地址**，同时通过出参拿到给硬件用的地址。

```c
void *
dma_alloc_coherent(struct device *dev, size_t size,
                   dma_addr_t *dma_handle, gfp_t flag);
/* 返回值  → CPU 用的虚拟地址（分配失败为 NULL）
 * dma_handle → 给硬件的 DMA 地址（出参）              */
```

**坑**：不能拿 `kmalloc` 出来的虚拟地址直接丢给 DMA——虚拟地址对 DMA 无效，硬件根本不认。

**精度修正（按 v6.6 内核文档核对）**：`dma_handle` 是 **DMA 地址（总线地址）**，**不严格等于物理地址**。官方 `Documentation/core-api/dma-api.rst` 原文：

> A CPU cannot reference a `dma_addr_t` directly because **there may be
> translation between its physical address space and the DMA address space**.

- 平台有 **IOMMU/SMMU** 时，DMA 地址是 IOMMU 映射后的 **IOVA**，和物理地址是两套空间（IOMMU 甚至能把物理上不连续的页映射成一个连续 DMA 地址段）。
- 树莓派这类无 SMMU 的简单 SoC 上，DMA 地址实践中就等于物理地址——所以直觉"只认物理地址"在嵌入式裸机上成立，但在服务器/现代 x86 上必须升级为"只认**总线地址**"。

### DMA 缓冲区

一块专门给**硬件和 CPU 双向交换数据**的内存：硬件写、CPU 读；或 CPU 写、硬件读。一致性分配（`dma_alloc_coherent`）出来的就是典型 DMA 缓冲区。

### 与 `copy_from_user` 的对比

| | `copy_from_user` | DMA |
|---|---|---|
| 执行者 | **CPU 亲自**做内存拷贝 | **硬件**做内存搬运 |
| 本质 | CPU 执行循环拷贝指令 | DMA 控制器独立搬运 |
| CPU 占用 | 占用（拷贝期间跑不了别的） | 不占用（并行执行其他代码） |

### 两种 DMA 模式（对应 12.1 / 12.2）

| | 一致 DMA（coherent） | 流式 DMA（streaming） |
|---|---|---|
| 内存来源 | **专用分配**（`dma_alloc_coherent`） | **复用已有内存**（`dma_map_single` 等） |
| Cache 处理 | 分配时就对 cache 做屏蔽/映射，CPU 与硬件**立刻**看到彼此的写 | 每次 map 时做 **cache 刷新/失效** |
| 性能 | 分配昂贵（部分平台最小一页），适合长期缓冲 | 性能更高，**网卡多用** |
| 典型用户 | 描述符环、控制块 | 收发包数据缓冲 |

官方定义（v6.6 dma-api.rst）：Consistent memory is memory for which a write by either the device or the processor can **immediately** be read by the other without worrying about caching effects.

```c
/* 流式：映射一块已有的 CPU 虚拟内存给设备用 */
dma_addr_t
dma_map_single(struct device *dev, void *cpu_addr, size_t size,
               enum dma_data_direction direction);
/* direction: DMA_TO_DEVICE / DMA_FROM_DEVICE / DMA_BIDIRECTIONAL */
```

---

## HFT / 嵌入式关联

| 路线 | 要点 |
|------|------|
| HFT | **网卡 DMA 把报文直接丢内存，CPU 省去拷贝开销 → 直接降低收包延迟**；DPDK 全程基于此（内核旁路 + DMA + 轮询模式收包） |
| 嵌入式 | 音频 ADC / SD 卡 / SPI 等低速外设靠 DMA 连续搬运数据流，CPU 只在每个"块"完工时被中断一次 |
| 低延迟线索 | 12.1 缓存一致性标注**精读（低延迟）**——coherent 内存牺牲 cache 性能换一致性，流式映射靠 flush/invalidate 保正确性，两条路线的延迟代价要分开量化 |

---

## 代码示例

（环境无 C 编译器，以下为按 v6.6 `Documentation/core-api/dma-api.rst` 人工核对过的骨架，未真机编译）

```c
#include <linux/dma-mapping.h>

/* 一致 DMA：收包描述符环 */
struct rx_ring {
    void       *cpu_addr;    /* CPU 侧虚拟地址 */
    dma_addr_t  dma_addr;    /* 硬件侧 DMA 地址 */
};

static int rx_ring_alloc(struct device *dev, struct rx_ring *r)
{
    r->cpu_addr = dma_alloc_coherent(dev, RING_SIZE,
                                     &r->dma_addr, GFP_KERNEL);
    if (!r->cpu_addr)
        return -ENOMEM;

    /* 硬件寄存器里写的是 r->dma_addr —— 绝不是 r->cpu_addr！
     * 坑：dma_alloc_coherent 只能在开中断的上下文调用 */
    iowrite32(r->dma_addr, dev_regs + RX_RING_BASE);
    return 0;
}

static void rx_ring_free(struct device *dev, struct rx_ring *r)
{
    dma_free_coherent(dev, RING_SIZE, r->cpu_addr, r->dma_addr);
    /* dev/size/dma_handle 必须与分配时完全一致；cpu_addr 传返回的虚拟地址 */
}
```

---

## 衔接

- 大纲：[../OUTLINE.md](../OUTLINE.md)
- 上一章：[Ch11 内核内存管理](../chapter-11-kernel-memory-management/notes.md)（`kmalloc` 的虚拟地址为什么不能给 DMA——本篇坑点的前置）
- 思想源头：[LDD3 Ch15 Memory Mapping and DMA](../../classic-driver-theory/chapter-15-memory-mapping-and-dma/notes.md)（2.6.10 老 API，思想精读）
- 中断侧：[LDD3 Ch10 Interrupt Handling](../../classic-driver-theory/chapter-10-interrupt-handling/notes.md)（DMA 完成中断的上半部/下半部处理）
- `copy_from_user`：[Ch4 字符设备驱动](../chapter-04-character-device-drivers/notes.md)
- HFT 落地：[13-dpdk](../../../13-dpdk/01-Intro-Book/)（DMA + 内核旁路的工程化）

---

## 代码自测

<details>
<summary>参考答案</summary>

**Q1：DMA 搬运内存的时候，CPU 在干什么？**

CPU 可以并行跑别的代码，不需要等待搬运；DMA 结束靠**中断**通知 CPU。

**Q2：为什么 DMA 不能用虚拟地址，只能用物理地址？**

设备发起的总线事务**不经过 CPU 的 MMU**——MMU 的页表翻译只对 CPU 发出的访存生效，硬件看到的地址就是总线地址，没有任何东西帮它做 VA→PA 翻译。
进阶修正：有 **IOMMU** 的平台上，设备地址会经过 IOMMU 翻译（IOVA→PA），所以现代说法是"设备只认** DMA 地址**"；无 IOMMU 的简单 SoC（如树莓派）上 DMA 地址就等于物理地址。

**Q3：`dma_alloc_coherent` 返回的虚拟地址能直接写给硬件寄存器吗？**

不能。写给硬件的必须是**出参 `dma_handle`** 里的 DMA 地址；返回值是给 CPU 访问用的虚拟地址。两者一个走 CPU/MMU，一个走设备总线，是两套地址空间。

</details>

---

## 参考

- Madieu, *Linux Device Drivers Development*, Chapter 12
- Linux v6.6 `Documentation/core-api/dma-api.rst`（本篇 API 签名与 coherent 定义均已核对）
