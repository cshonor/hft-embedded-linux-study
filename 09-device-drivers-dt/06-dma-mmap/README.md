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

```c
static int my_mmap(struct file *filp, struct vm_area_struct *vma)
{
    /* 一次性把整段物理区间映射过去 */
    return remap_pfn_range(vma, vma->vm_start,
                           virt_to_phys(buf) >> PAGE_SHIFT,
                           vma->vm_end - vma->vm_start,
                           vma->vm_page_prot);
}
```

用户态 `mmap(fd, ...)` 之后直接读写，**不再有 `read()` 的一次拷贝**。

---

## 本节笔记

| # | 笔记 |
|---|------|
| 6.1 | [DMA 基础](./6.1-dma-fundamentals.md) |
| 6.2 | [缓存一致性](./6.2-cache-coherence.md) |
| 6.3 | [一致性与流式映射](./6.3-coherent-streaming-mappings.md) |
| 6.4 | [散聚传输](./6.4-scatter-gather.md) |

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
