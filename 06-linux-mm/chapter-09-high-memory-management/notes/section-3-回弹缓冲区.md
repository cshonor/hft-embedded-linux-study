# Ch 9 §3 回弹缓冲区 (Bounce Buffers)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**
> 源码核验：Linux **v6.6**（`mm/bounce.c` **已删除**，由 `kernel/dma/swiotlb.c` 取代）

---

## 本节讲什么

本节回答一个问题：**设备 DMA 够不着某个物理页时，内核怎么办？**

原书答案是「块层 bounce buffer」（`mm/bounce.c` + `blk_queue_bounce`）；但 **v6.6 里 `mm/bounce.c` 已删除（404）**，取而代之的是 **swiotlb（软件 IO TLB）**。本节先讲「为什么要 bounce」这个不变的动机，再落到 v6.6 的 swiotlb 真身。

---

## 1. 动机不变：设备「看不见」高物理地址

**场景**：设备 DMA 只能寻址**有限的物理地址范围**（32 位设备接 64 位机、PAE、部分旧控制器），而目标页的物理地址**超出设备的 `dma_mask`**：

```
设备 DMA 写 ──► 低地址 bounce buffer（设备可见区）
                    │
                    ▼ 内核 memcpy（swiotlb_bounce）
              目标页（物理地址超出 dma_mask）
```

| 方向 | 流程 |
|------|------|
| **设备 → 内存（读盘/网卡入）** | 设备 DMA 到 bounce → `swiotlb_bounce` **复制**到目标页 |
| **内存 → 设备（写）** | 从目标页 **复制到 bounce** → 设备 DMA |

**代价**：**多一次完整拷贝**——但仍可能比「为腾出低地址内存而 swap/搬迁整个进程」便宜。这个「两害相权」的取舍没变。

---

## 2. v6.6 真相：`mm/bounce.c` 已删除

原书的块层 bounce 缓冲（`blk_queue_bounce` + `mm/bounce.c`）在 v6.6 **已经不存在**：

```
$ curl .../v6.6/mm/bounce.c
404: Not Found        ← 文件已删除
```

**为什么删？** 三个原因：

| 原因 | 说明 |
|------|------|
| HIGHMEM 变罕见 | 块层 bounce 主要服务 HIGHMEM；64 位普及后 HIGHMEM 几乎绝迹 |
| swiotlb 上位 | 「设备够不着内存」的场景统一交给 DMA 层的 **swiotlb** 处理 |
| 块层瘦身 | `blk_queue_bounce` 是块层老包袱，删掉让 bio 路径更干净 |

**结论**：原书 §3 的机制（块层 bounce）是**历史遗迹**，但「设备地址可达性」这个**问题**依然真实，只是**解决它的位置从块层移到了 DMA 层**。

---

## 3. v6.6 真身：swiotlb（软件 IO TLB）

swiotlb 在 **DMA 映射层**拦截「设备够不着」的请求，用一块**启动时预分配的低地址内存**做 bounce：

```c
/* kernel/dma/swiotlb.c —— 关键对象 */
struct io_tlb_pool {
    unsigned long start;          /* 池的物理起始地址（低地址区） */
    unsigned long nslabs;         /* 池里的槽位数 */
    ...
};
static struct io_tlb_mem io_tlb_default_mem;   /* 全局默认池 */

/* :827 真正的拷贝 */
static void swiotlb_bounce(struct device *dev, phys_addr_t tlb_addr,
                           size_t size, enum dma_data_direction dir);
```

| 概念 | 对应原书 bounce |
|------|----------------|
| `io_tlb_default_mem`（预分配低地址池） | bounce buffer 区 |
| `swiotlb_bounce()` | bounce 的 memcpy |
| `swiotlb_tbl_map/unmap()` | 分配/释放 bounce 槽 |
| `phys_limit`（= `DMA_BIT_MASK(32)` 或 `ARCH_LOW_ADDRESS_LIMIT`） | 「设备可见区」的上限 |

**工作流**（简化）：

```
dma_map_page(dev, page, ...)
  └─ dma_direct_map_page() 检查 page 物理地址 ≤ dev->dma_mask？
       ├─ 是 → 直接映射，设备 DMA 目标页本身（无拷贝）
       └─ 否 → swiotlb_tbl_map() 从 io_tlb_default_mem 拿低地址槽
              → 设备 DMA 到 bounce 槽
              → dma_unmap 时 swiotlb_bounce() 拷回/拷出目标页
```

**关键**：swiotlb 只在「**页物理地址超出设备 `dma_mask`**」时才触发。现代 NIC 的 `dma_mask` 通常是 64 位，**几乎不触发**；只有那些 `dma_mask = 32 位` 的旧设备/虚拟化场景才走 swiotlb。

---

## 4. HFT / 嵌入式关联（原书对照的现代版）

| 现象 | 现代机制的兑现 |
|------|----------------|
| NIC DMA 到 registered 物理地址 | 必须**在设备 `dma_mask` 内**分配缓冲（ibverbs/DPDK memzone），否则走 swiotlb 多一次拷贝 |
| 延迟尖刺排查 | 若 `dmesg` 出现 swiotlb 相关告警，说明有 DMA 落到 bounce，**每次 I/O 多一次 memcpy** |
| 虚拟化（VM） | virtio 设备常配 32 位 dma_mask，swiotlb 是**常态路径**，`swiotlb=` 启动参数可调池大小 |
| 树莓派/嵌入式 | 设备 DMA 引擎有地址限制，理解 swiotlb 能定位「DMA 为什么慢」 |

**观测点**：`dmesg | grep -i swiotlb` 看池大小与溢出告警；`/proc/meminfo` 或 `swiotlb=` 内核参数控制池容量（默认 64MB）。

---

## 5. 衔接

- 下节 [§4 紧急内存池](./section-4-紧急内存池.md)：bounce 需要「内存再紧也要能拿到」的保底
- DMA 机制：[09-device-drivers-dt Ch12 DMA](../../../09-device-drivers-dt/modern-driver-practice/chapter-12-dma/)（`dma_need_sync`、`dma_map_ops`）

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：原书的块层 bounce 和 v6.6 的 swiotlb 有什么区别？**
A：**位置和触发点不同**。块层 bounce（`blk_queue_bounce`）在**块设备队列**上，针对「bio 页是 HIGHMEM」；swiotlb 在 **DMA 映射层**（`dma_map_page` 路径），针对「页物理地址超出设备 `dma_mask`」。前者随 HIGHMEM 消亡而被删，后者作为通用「设备可达性」兜底存活至今。

**Q2：`mm/bounce.c` 在 v6.6 已删除，那原书的「bounce 多一次拷贝」代价还存在吗？**
A：**机制变了，代价仍在**——只要设备 `dma_mask` 够不着目标页，swiotlb 就要 `swiotlb_bounce()` 多一次 memcpy。只是触发条件从「HIGHMEM」变成了「物理地址超出 dma_mask」。现代 64 位 NIC 几乎不触发，旧设备/虚拟化仍常见。

**Q3：swiotlb 的池是什么时候、怎么分配的？**
A：**启动时预分配**一块低地址物理内存（`io_tlb_default_mem`），默认 64MB，可用内核参数 `swiotlb=` 调整。预分配是关键——运行时内存紧张时，bounce 也**必须有现成的低地址槽**可用，否则设备 I/O 会因拿不到 bounce 而失败。

**Q4：怎么判断「我的系统是否在走 swiotlb」？**
A：`dmesg | grep -i swiotlb` 看池大小和告警（如「swiotlb buffer is full」）。更精确：观察 DMA 相关计数器，或对可疑设备看其 `dma_mask`（`/sys/.../dma_mask_bits`）。若设备 dma_mask 是 32 位而系统内存 >4GB，高概率走 swiotlb。

**Q5：HFT 里为什么 DPDK/ibverbs 强调「在 dma_mask 内分配 registered 内存」？**
A：因为一旦缓冲的物理地址超出设备 `dma_mask`，DMA 就**不能直接落进你的缓冲**，而要经 swiotlb 多一次拷贝——这在数据面是**不可接受的延迟 + CPU 开销**。所以 RDMA/DPDK 显式控制**物理内存分配**（hugepage + 物理连续），确保**设备直接 DMA 到最终缓冲**，杜绝 bounce。

</details>

---
