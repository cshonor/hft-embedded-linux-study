# Ch 9 §5 2.6 内核的新变化（及 HIGHMEM 体系的终结）

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**
> 源码核验：Linux **v6.6**（`mm/bounce.c` 删除 / `kmap_atomic` 废弃 / `kmap_local_page` 上位）

---

## 本节讲什么

本节回答一个问题：**从原书 2.6 到 v6.6，高端内存管理这条线发生了什么大事？**

答案是：原书 §5 的「小变化」（mempool 泛化、删 `page->virtual`）之后，**整个 HIGHMEM 体系在 64 位时代走向终结**——bounce 删除、`kmap_atomic` 废弃、`kmap_local_page` 上位。这是本章真正的「新变化」。

---

## 1. 原书的 2.6 变化（仍是事实）

| 变化 | 说明 |
|------|------|
| **通用 `mempool`** | 2.4 HIGHMEM 专用 emergency pool → 2.6 `mempool_t` 全内核可用 |
| **移除 `page->virtual`** | 2.4 在 `struct page` 存 PKMap 的 vaddr——海量 page 结构浪费内存；2.6 删除，改用 `page_address()` 哈希表跟踪 |

**`page->virtual` 移除的意义**（HFT 关心）：`struct page` 是**按物理页数量复制的**——每 4KB 一个。一个 8 字节指针字段 × 几十 GB 内存的页数 = **数百 MB 纯浪费**。删掉它，`struct page` 更紧凑，`mem_map`/vmemmap 内存开销大降。这跟 Ch2 §3「`struct page` 5-word union 字段复用」是同一套「**page 结构必须紧凑**」的追求。

---

## 2. 2.6 → v6.6：HIGHMEM 体系的终结

| 时间线 | 事件 |
|--------|------|
| 2.6 | `page->virtual` 删除；mempool 泛化 |
| 5.x | `kmap_atomic` 标记废弃（`kmap_local_page` 上位） |
| 5.9 前后 | 块层 bounce（`mm/bounce.c` + `blk_queue_bounce`）删除 |
| **v6.6** | `mm/bounce.c` 已不存在；`kmap_atomic` 仍是「Deprecated」；HIGHMEM 只在 32 位/PAE 存活 |

**三件大事的落点**：

| 原书概念 | v6.6 归宿 |
|----------|----------|
| `kmap()` / `kunmap()` | 遗留 API，被 `kmap_local_page()` 取代（§2） |
| `kmap_atomic()` | **已废弃**，「Do not use in new code」（§2） |
| 块层 bounce | **已删除**，由 swiotlb 接管「设备可达性」问题（§3） |
| bounce 紧急池 | **已删除**，swiotlb 启动预分配 + mempool 泛化接管（§4） |
| `page->virtual` | 已删除，`page_address()` 哈希表跟踪（本节） |

---

## 3. 为什么 64 位让 HIGHMEM 体系「整体退休」

一条因果链说清：

```
64 位地址空间（128TB 直接映射区）足够覆盖全部物理内存
        │
        ▼
没有「内核够不着」的 HIGHMEM 页（CONFIG_HIGHMEM 未定义）
        │
        ▼
不需要 PKMap 窗口（§1）、不需要 kmap 临时映射（§2）
        │
        ▼
不需要「HIGHMEM 页做 DMA」的 bounce（§3）
        │
        ▼
HIGHMEM 整条线（PKMap/kmap/bounce/紧急池）在 64 位上全部退场
```

**结论**：Ch9 整章描述的是**32 位时代的「高端内存」补丁体系**。理解它的价值在于**「临时映射」「bounce」「预留」这三个思想**——它们在 64 位上以 `kmap_local_page`、swiotlb、mempool 的形式**换了皮继续活着**，但「HIGHMEM」这个具体的物理前提已经消失。

---

## 4. HIGHMEM 访问路径一图（32 位，历史对照）

```
HIGHMEM struct page
        │
        ├─ 内核要读写内容
        │      kmap / kmap_atomic → PKMap VA → 访问
        │      kunmap / kunmap_atomic
        │      （v6.6：kmap_local_page 取代前两者）
        │
        └─ 设备 DMA
               bounce buffer (LOWMEM) ↔ memcpy ↔ HIGHMEM page
               （v6.6：mm/bounce.c 删除，改 swiotlb）
               emergency pool 保底 bounce 分配
               （v6.6：删除，swiotlb 启动预分配 + mempool）
```

---

## 5. HFT / 阅读建议

| 读者 | 建议 |
|------|------|
| **x86_64 HFT** | **跳过正文**；带走三思想——临时映射、bounce、预留——的**现代形态**（`kmap_local_page`/swiotlb/mempool） |
| **嵌入式 / 32 位** | 精读 `kmap`/bounce，它们仍是真实路径 |
| **虚拟化** | swiotlb 是常态路径，`swiotlb=` 参数与 `dmesg` 告警值得关注 |
| **继续** | [Ch10 页框回收](../../chapter-10-page-frame-reclamation/) |

---

## 6. 衔接

- 下一章：[Ch10 页框回收](../../chapter-10-page-frame-reclamation/)（内存紧张时的回收，与 §4 预留互补）
- HIGHMEM 来由：[Ch2 §4 高端内存](../../chapter-02-describing-physical-memory/notes/section-4-高端内存.md)

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：`page->virtual` 删除后，内核怎么知道「某 HIGHMEM 页映射在哪」？**
A：用 **`page_address()` 哈希表**（`page_address_htable`）替代。`kmap_high()` 映射成功时 `set_page_address(page, vaddr)` 把「page → vaddr」记进哈希表，`kunmap_high()` 时清掉。非 HIGHMEM 页直接算 `PAGE_OFFSET + pfn`，不查表。这样 `struct page` 不用为每个页都存一个 vaddr 指针。

**Q2：为什么删 `page->virtual` 能省「数百 MB」？**
A：因为 `struct page` 按**物理页数量**复制——每 4KB 物理内存一个。一个 8 字节指针 × （64GB / 4KB = 1600 万页）≈ 128MB，且**绝大多数页永远用不到 PKMap**（只有 HIGHMEM 页才需要存 vaddr）。所以「给每页都存 vaddr」是纯浪费，改成「哈希表只为真正映射的页存」就省下来了。

**Q3：64 位让 HIGHMEM 体系退休的「根因」是哪一环？**
A：**直接映射区足够大**。64 位内核 `PAGE_OFFSET` 之上有 128TB 虚拟窗口，远超物理 RAM，所以**每一页物理内存都有固定的直接映射虚拟地址**，不存在「内核够不着」的页。HIGHMEM 的前提（内核地址空间不够覆盖物理内存）消失，整条补丁体系（PKMap/kmap/bounce）自然失去存在意义。

**Q4：`kmap_atomic` 和块层 bounce 都「删了/废了」，是不是说明「临时映射」「bounce」这些需求本身消失了？**
A：**不是**。需求还在，只是**换了载体**：临时映射的需求由 `kmap_local_page` 承接（更快的 per-CPU 实现）；bounce 的需求由 swiotlb 承接（在 DMA 层而非块层）。删的是「过时的实现」，不是「需求」。理解这一点，才能看懂「为什么 64 位上这些概念还能遇到」。

**Q5：本章五个「跳过 ⚪」的小节，HFT 读者到底该带走什么？**
A：**三个思想 + 它们的现代形态**：①「临时内核 VA 窗口」→ `kmap_local_page`（64 位上是 no-op）；②「设备够不着就 bounce」→ swiotlb（只在 dma_mask 受限时触发）；③「关键路径预留」→ mempool。这三个思想在延迟敏感系统里都有直接对应（避免 kmap 开销、避免 swiotlb 拷贝、预留关键对象），比背 HIGHMEM 的历史细节有价值得多。

</details>

---
