# Ch 3 §7 2.6 内核的新变化 (What's New in 2.6) → v6.6 演进时间线

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**
> 源码核验：Linux **v6.6**（`mm/rmap.c`、`mm/nommu.c`、`mm/memory.c`）

---

## 本节讲什么

原书每章末的"What's New in 2.6"是 2004 年的"现代"；本节把它接到 v6.6，给出一条 **可检索的演进时间线**——读旧书时随时回来对照"这条机制后来变成了什么"。

---

## 1. 无 MMU 架构（`mm/nommu.c`）

为无 MMU 微控制器提供 nommu 路径——无硬件页表，地址即物理地址（`MAP_FIXED` 强制对齐）。v6.6 里 `CONFIG_MMU=n` 仍然存在（部分 blackfin/nommu arm），**与 HFT 服务器无关**，但嵌入式支线会再遇到：uClinux 风格设备上 `fork` 不可用（无 COW）、mmap 只能映射设备已暴露的物理段。

## 2. 反向映射（rmap）——重点，至今仍是核心

| 2.4 痛点 | 2.6 rmap |
|----------|----------|
| 共享页换出时需 **扫描所有进程页表** 找指向该页的 PTE——O(进程×页表) | 每个 `struct page` 挂 **映射它的 PTE 反向链** |
| 解除映射慢、难原子 | 直接定位并解除所有 PTE，pageout 快得多 |

**v6.6 现状（`mm/rmap.c`）：** 链表对象已不是 PTE 而是 **`struct anon_vma_chain`（AVC）**：

```
物理页 page ──► anon_vma(chain) ──► AVC₁(mm₁/vma₁) AVC₂(mm₂/vma₂) …
                    ▲                   │
              锁：rcu_read_lock         ▼ 每链再进 vma 的区间定位 PTE
```

| rmap 消费者 | 干什么 |
|-------------|--------|
| `page_referenced()` | 聚合各 PTE 的 accessed 位 → 回收年龄判断 |
| `try_to_unmap()` | 换出前解除全部映射（含 TLB flush） |
| `page_move_account`/迁移 | THP defrag、NUMA balance 的前提 |
| `folio_mkclean` | 写回/去脏 |

**HFT 记住一条：** 共享映射多的页（`MAP_SHARED` 行情快照、tmpfs）rmap 链长 → 换出/迁移成本高。全 pin（mlock/大页）是绕开 rmap 税的唯一干净办法。

## 3. 高端内存中的 PTE（PTEs in High Memory）

2.6 允许 PTE 表页分在 ZONE_HIGHMEM，访问时 `pte_offset_map()` 临时 kmap。**64 位上 HIGHMEM 已死**（ARM64/x86_64 无此区），但 API 骨架留在 `pte_offset_map/unmap` 配对里（§2 已讲）。Ch 9 高端内存整章在 64 位语境下按"历史机制"读。

## 4. 2.4 → 2.6 → v6.6 页表机制演进时间线

| 机制 | 2.4 | 2.6 | v6.6 |
|------|-----|-----|------|
| 页表层级 | 三级（x86-32） | 四级（x86-64 出现） | 3/4/5 级可配置，P4D 折叠 |
| 表页缓存 | quicklist | slab/kmem_cache | ptdesc + per-CPU pcp + RCU 释放 |
| TLB 失效 | 逐操作同步 flush | mmu_gather 批量 | + RCU_TABLE_FREE、PCID/ASID 常态化 |
| rmap | 无（2.4 直接 pte 链） | anon_vma | anon_vma_chain + folio 化 |
| THP | 无 | 2.6.38 引入 | THP + khugepaged；mTHP（多尺寸）6.8+ |
| 用户faultfd | 无 | 2.6 后期无 | PTE marker 支持 uffd-wp 持久化 |
| VMA 树 | rbtree+链表 | 同 | **maple tree**（06.5/ch05） |
| 页表锁 | mm 级粗锁 | split ptlock（CONFIG_SPLIT_PTLOCK_CPUS） | 同，4KiB 表页一把锁 |

**读旧书的换算规则：** 遇到 `pte_offset` → `pte_offset_map/_unmap`；遇到 mem_map → vmemmap；遇到 quicklist → slab/pcp；遇到 page->mapping 直接反查 → 走 AVC。

## 5. HFT 精读 checklist

| 主题 | 行动 |
|------|------|
| TLB / 页表 walk | 压工作集、大页、减少 pointer chasing 数据结构 |
| PTE present + mlock | 避免缺页与 swap 路径 |
| `flush_tlb_*` | 理解绑核、减少跨核页表改动 |
| rmap | 理解共享内存 / mmap 文件换出成本——多映射页更「重」 |
| THP | 在减少 TLB miss 与合并延迟抖动之间取舍 → [note-透明大页THP](./note-透明大页THP.md) |
| split ptlock | 多线程并发缺页的可扩展性上限（每表页一把 `ptdesc->ptl`） |

## 6. 衔接

- [Ch 10 页框回收](../../chapter-10-page-frame-reclamation/)：rmap 的主战场
- [Ch 12 共享内存](../../chapter-12-shared-memory-virtual-filesystem/)：文件页 rmap 经 address_space
- [06.5](../../../06.5-modern-mm/)：上面时间线右列的逐项展开

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：2.4 的 rmap 原型（page→pte 直接链）为什么被 anon_vma_chain 替换？**
A：fork 爆炸。直接 PTE 链在 fork 链式共享时每个 page 的链表长度失控（O(fork 深度×vma)）。AVC 两级结构让"链"按 (anon_vma, vma) 去重合并，同一 fork 组共享一条 anon_vma，遍历成本可控。这是面试高频题的内核版答案。

**Q2：`CONFIG_SPLIT_PTLOCK_CPUS` 是什么？**
A：split page table lock：CPU 数超过该值（默认 4）时启用——每个 PTE 表页自己一把锁（藏在 ptdesc），而不是整个 mm 一把 page_table_lock。多线程引擎并发缺页/GUP 的可扩展性直接受益。64 位 SMP 上默认开。

**Q3：v6.6 读 `/proc/pid/maps` 和读旧书讲的 rbtree 遍历一样吗？**
A：数据结构换成了 maple tree（range B-tree），接口（mmap_lock 读 + mas_find）不同但语义一致。性能差异：查找 O(log n) 且 cache 友好；并发写锁粒度更细。详见 06.5/ch05。

**Q4：nommu 内核上跑 DPDK/AF_XDP 类零拷贝有意义吗？**
A：nommu 设备（MCU 级）本来就没有用户/内核隔离，"零拷贝"退化为物理地址直访问。AF_XDP/DPDK 依赖的页表 pin、GUP、iommu 在 nommu 上都无意义。嵌入式支线遇到时按"物理直访+寄存器映射"理解。

**Q5：THP 是 2.6.38 引入的，为什么说 mTHP 是 6.8+？两者差别？**
A：THP 只有 2MiB（PMD 级）一种"透明"尺寸；mTHP（multi-size THP）允许 16K/32K/.../1M 的中间尺寸按 folio 一次映射。v6.6 尚无 mTHP（`/sys/kernel/mm/transparent_hugepage/` 下无 anon 分档目录）。写 HFT 建议时别把 mTHP 特性算进 6.6。

</details>

---
