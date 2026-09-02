# 附录 B 描述物理内存 · Describing Physical Memory

> **Code Commentary** · Mel Gorman · **选读** · 源码核验：Linux v6.6

概念总览 → [./chapter-02-describing-physical-memory/](./chapter-02-describing-physical-memory/)

---

## 本节走读什么

正文 Ch2 讲了「Node / Zone / Page 是什么、为什么分三层」。本附录走读**这三层数据结构在 v6.6 里的真实代码组织**：`struct pglist_data`（mmzone.h:1261）、`struct zone`（:810）、`struct page`（mm_types.h:74）+ `struct folio`（:293）。

聚焦「代码怎么组织」，不重复 Ch2 的机制讲解。

---

## 1. 三层结构的「包含关系」

```
struct pglist_data（pg_data_t）    // mmzone.h:1261  —— 一个 NUMA 节点
    ├─ node_zones[]                // 该节点的 ZONE_DMA/DMA32/NORMAL/MOVABLE...
    │      └─ struct zone          // mmzone.h:810   —— 一个内存区域
    │             └─ _watermark[]  // 四水位（Ch2）
    ├─ node_mem_map                // 指向 struct page 数组（该节点的所有页框）
    │      └─ struct page          // mm_types.h:74   —— 一个物理页框
    └─ node_zonelists[]            // 回退顺序（分配时按序找 zone）
```

**核心设计**：`struct page` 是**最底层、数量最大**（每个物理页一个），所以被极致压缩——用 **union 复用**字段（`struct page` 里 5 个 word 的 union 复用全景见 Ch2 §3）；`struct zone` 和 `pglist_data` 是**每节点/每区域少量**的元数据，字段可以铺开。

## 2. `struct page` 的字段复用（mm_types.h:74）

v6.6 的 `struct page` 用了**一个巨型 union**，把「不同用途的页」复用同一块内存：

```c
struct page {
    unsigned long flags;                    // ① 页标志（PG_locked/PG_dirty...），低 bit 编码 section/node/zone
    union {
        struct {                            // ② 匿名页 / 普通页
            struct address_space *mapping;  //    所属地址空间
            pgoff_t index;                  //    文件偏移
            ...
        };
        struct {                            // ③ 空闲页（Buddy）
            unsigned long private;          //    低 bit 编码 order
        };
        ...
    } _mapcount;                            // 4-word 的 union（详情见 Ch2 §3 全景图）
    atomic_t _refcount;                     // 引用计数
    ...
};
```

**走读要点**：`struct page` 的字段几乎全是**按「当前页处于什么状态」来解读的**——同一块内存，Buddy 空闲时 `private` 存 order，被页缓存用时 `mapping/index` 存文件信息。这是「省内存」和「难读」的一体两面。

## 3. `struct folio`：v6.6 的新抽象（mm_types.h:293）

```c
struct folio {
    struct page page;   // 内嵌第一个 struct page（头页）
};
```

**为什么要 folio**：原来「一页」和「一个复合页（order-N）」都用 `struct page`，`compound_head()` 要不停反查头页，易错。folio 把「**以 page 为单位的所有权/记账**」抽出来，`struct page` 退化为 folio 的「物理页视图」。这是 v5.16+ 逐步推进的**大重构**（Ch2 §3 有全景对比）。

## 4. `struct zone` 的关键字段（mmzone.h:810）

| 字段 | 作用 |
|------|------|
| `_watermark[NR_WMARK]` | 四水位（WMARK_MIN/LOW/HIGH/PROMO，Ch2） |
| `free_area[MAX_ORDER]` | Buddy 空闲链表数组（Ch6 分配从这里拿） |
| `spanned_pages` / `present_pages` / `managed_pages` | ⭐ 三计数（Ch2 详解：空洞/热插拔/可管理） |
| `node` | 回指针到 pglist_data |
| `vm_stat[]` | per-zone 计数（配合 per-CPU 批量） |

**走读要点**：`_watermark` 和 `free_area` 是**分配器的直接消费者**（page_alloc 看水位决定是否回收、从 free_area 拿页），所以 `struct zone` 是「物理内存描述」与「分配策略」的**连接点**。

## 5. `struct pglist_data` 的关键字段（mmzone.h:1261）

| 字段 | 作用 |
|------|------|
| `node_zones[MAX_NR_ZONES]` | 该节点的 zone 数组 |
| `node_zonelists[MAX_ZONELISTS]` | ⭐ 回退顺序（分配失败往哪找） |
| `node_mem_map` | `struct page` 数组基址 |
| `kswapd` / `kswapd_wait` | 每节点的回收线程（Ch10） |
| `node_id` | 节点号 |

**走读要点**：`node_zonelists` 是 NUMA 分配的核心——它决定了「本节点内存不够时，按什么顺序去别的节点找」，Ch2 的 `numactl` 示例最终就落到这个字段。

---

## 与正文对应

| 附录内容 | 正文落点 |
|----------|----------|
| `struct page` union 复用 | Ch2 §3（5-word 复用全景图 + PageTail bit0 借位） |
| `struct folio` | Ch2 §3（folio 引入动机） |
| `struct zone` 三计数/水位 | Ch2 §2（spanned/present/managed + 四水位） |
| `pglist_data` / zonelist | Ch2 §1（节点/回退顺序） |

---

## HFT / 嵌入式关联

**延迟定位入口**：`struct zone` 的 `free_area` 和 `_watermark` 是「分配是否会触发回收」的直接判据——HFT 里若 `/proc/zoneinfo` 显示某个 zone 的 free 逼近 WMARK_MIN，说明该 node 即将进入 direct reclaim 尖刺区，应提前 `numactl` 换 node 或扩容。

---

## 相关章节

- 上一章：[appendix-A-简介.md](./appendix-A-简介.md)
- 下一章：[appendix-C-页表管理.md](./appendix-C-页表管理.md)

---

<details>
<summary>自测 5 问（点开看答案）</summary>

**Q1：`struct page` 为什么用 union 复用字段？代价是什么？**

因为每个物理页一个 `struct page`，数量巨大，必须极致省内存；代价是「同一块内存按页状态解读」，可读性差、易错（这也是 folio 出现的原因之一）。

**Q2：`struct folio` 和 `struct page` 什么关系？**

`struct folio` 内嵌第一个 `struct page`（头页），把「以 page 为单位的记账/所有权」抽象出来，`struct page` 退化为 folio 的物理页视图。

**Q3：`struct zone` 的 `free_area[MAX_ORDER]` 和 `_watermark[]` 分别被谁消费？**

都被 `page_alloc.c` 消费：`free_area` 是 Buddy 空闲链表（分配从这里拿页），`_watermark` 决定「是否触发回收」（Ch6/Ch10）。

**Q4：`pglist_data` 的 `node_zonelists` 字段是干什么的？**

存分配失败时的**回退顺序**——本节点内存不够时按什么顺序去其他节点找，是 NUMA 分配策略的落点。

**Q5：`struct zone` 的三计数 `spanned_pages` / `present_pages` / `managed_pages` 区别？**

spanned = 覆盖的物理范围（含空洞）、present = 实际存在的页（扣除热插拔空洞）、managed = 可被 Buddy 管理的页（再扣掉预留）。详见 Ch2 §2。

</details>
