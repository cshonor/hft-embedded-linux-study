# 06 · DMA 与 mmap：让数据不经过 CPU

> **本节讲什么：** 大批量数据怎么从外设进内存，以及怎么把内核内存映射到用户态让应用直接读。
> **核心矛盾：** DMA 设备写内存时**不经过 CPU 缓存**，于是产生缓存一致性问题——这是本节真正的难点。

---

## 为什么需要 DMA

| 方式 | 谁搬数据 | 代价 |
|------|---------|------|
| **PIO**（CPU 读寄存器） | CPU 一个字节一个字节搬 | 占满 CPU，速率低 |
| **DMA** | DMA 控制器直接写内存 | CPU 只做开始/结束，但引入**缓存一致性**问题 |

DMA 完成后发中断 → 驱动在中断/下半部里处理数据（接 [05-irq-locking](../05-irq-locking/)）。

---

## 缓存一致性（最关键的坑）

```
CPU 写数据 → 可能还留在 cache 里没写回内存
       → DMA 从内存读 → 读到旧数据 ✗

DMA 写数据到内存 → CPU 从 cache 读到旧值 ✗
```

| 映射类型 | 用途 | 一致性 |
|---------|------|--------|
| **一致性（coherent）映射** | 控制结构、描述符环（频繁双向、量小） | 硬件保证或禁缓存，`dma_alloc_coherent` |
| **流式（streaming）映射** | 一次性大块数据 | 需手动 `dma_map_single` / `dma_unmap_single`，配合 `dma_sync_*` |

**口诀：** 长期存活、双向访问用 coherent；一次性传大块用 streaming，传完立刻 unmap。

---

## 关键 API

```c
/* 一致性：描述符环之类的常驻结构 */
desc = dma_alloc_coherent(dev, size, &dma_handle, GFP_KERNEL);

/* 流式：一次传输 */
dma_addr = dma_map_single(dev, cpu_addr, size, DMA_TO_DEVICE);
/* ... 启动 DMA ... */
dma_unmap_single(dev, dma_addr, size, DMA_TO_DEVICE);

/* 散聚（scatter-gather）：物理不连续的多块 */
dma_map_sg(dev, sglist, nents, DMA_FROM_DEVICE);
```

---

## mmap：把内核内存交给用户态

**两种情况写法不同，别混用：**

```c
/* ① DMA 缓冲 —— 用 dma_mmap_coherent，它会自动设对缓存属性 */
static int my_mmap_dma(struct file *filp, struct vm_area_struct *vma)
{
    return dma_mmap_coherent(m->dev, vma, m->cpu_addr,
                             m->dma_addr, m->size);
}

/* ② MMIO 寄存器 / 保留内存 —— 手写 remap_pfn_range，但必须做两件事 */
static int my_mmap_mmio(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long off  = vma->vm_pgoff << PAGE_SHIFT;
    unsigned long size = vma->vm_end - vma->vm_start;

    if (off + size > m->region_size)          /* ★ ① 边界检查：vm_pgoff 是用户可控的 */
        return -EINVAL;

    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);   /* ★ ② 必须关缓存 */

    return remap_pfn_range(vma, vma->vm_start,
                           (m->phys_addr + off) >> PAGE_SHIFT,
                           size, vma->vm_page_prot);
}
```

⚠️ **最常见的错：DMA 缓冲也用 `remap_pfn_range` 且不动 `vm_page_prot`。**
默认 prot 是 **cached**，而 `dma_alloc_coherent` 的内核侧映射是 noncached——两侧属性不一致，
症状是**数据偶尔不对**（不是必崩，极难查）。详见 [6.5](./6.5-mmap-remap-pfn-range.md) 与 [6.6](./6.6-mmap-caching-and-fault.md)。

用户态 `mmap(fd, ...)` 之后直接读写，**不再有 `read()` 的一次拷贝**。记得用 `MAP_SHARED`。

---

## 本节笔记

| # | 笔记 |
|---|------|
| 6.1 | [DMA 基础](./6.1-dma-fundamentals.md) |
| 6.2 | [缓存一致性](./6.2-cache-coherence.md) |
| 6.3 | [一致性与流式映射](./6.3-coherent-streaming-mappings.md) |
| 6.4 | [散聚传输](./6.4-scatter-gather.md) |
| 6.5 | [mmap 与 remap_pfn_range](./6.5-mmap-remap-pfn-range.md) ★ 三种 API 选型 |
| 6.6 | [缓存属性与惰性 fault](./6.6-mmap-caching-and-fault.md) ★ MMIO 必须 noncached |

---

## HFT / 嵌入式关联

- **`mmap` + 轮询 = 零拷贝数据面**，这正是 DPDK / VFIO 旁路的内核侧基础（[13-dpdk](../../13-dpdk)）。用户态直接看到网卡 DMA 写的内存，没有 syscall、没有拷贝。
- **缓存一致性在 HFT 是日常**：用户态无锁队列 + DMA 场景下，一个漏掉的 `dma_sync_single_for_cpu` 就是"偶发读到旧数据"的灵异 bug 来源。
- **描述符环（ring buffer）** 这结构在网卡、DMA、eBPF ringbuf、HFT 无锁队列里反复出现，值得单独吃透（对照 [06.7-bpf-observability](../../06.7-bpf-observability) 的 ring buffer）。

---

## 验收

- [ ] 能解释为什么 DMA 会带来缓存一致性问题
- [ ] 能区分 coherent 与 streaming 映射并正确选型
- [ ] 用 `remap_pfn_range` 做过一次 mmap，用户态读到数据
- [ ] 知道 `dma_sync_*` 该在什么时候调用

---

## 衔接

- **上一步：** [05-irq-locking](../05-irq-locking/)
- **下一步：** [10-motion-control](../../10-motion-control)（传感器数据流）或回 [14-hft-engineering](../../14-hft-engineering)
- **卡住查书：** Madieu Ch11–12 · LDD3 Ch15、Ch8
