# 附录 E 启动内存分配器 · Boot Memory Allocator

> **Code Commentary** · Mel Gorman · **跳过** · 源码核验：Linux v6.6

概念总览 → [./chapter-05-boot-memory-allocator/](./chapter-05-boot-memory-allocator/)（现代源码对照 **memblock**）

---

## 本节走读什么

原书附录 E 走读 **bootmem**（位图分配器）。但 **bootmem 已在 v4.20 删除**（Ch5 已核验），v6.6 只剩 **memblock**（`mm/memblock.c`，64KB）。本附录走读 memblock 的代码组织——它是「区间表」而非「位图」。

---

## 1. 三结构体（include/linux/memblock.h）

```c
enum memblock_flags {                     // memblock.h:44
    MEMBLOCK_NONE, MEMBLOCK_HOTPLUG, MEMBLOCK_MIRROR, MEMBLOCK_NOMAP,
};

struct memblock_region {                  // memblock.h:59  一段区间
    phys_addr_t base;  phys_addr_t size;  // 起址 + 大小
    enum memblock_flags flags;  int nid;  // 属性 + NUMA 节点
};

struct memblock_type {                    // memblock.h:76  一类区间集合
    unsigned long cnt, max;               // 已用 / 容量
    phys_addr_t total_size;               // 总大小
    struct memblock_region *regions;      // 区间数组（可动态扩容）
};

struct memblock {                         // memblock.h:91  全局单例
    struct memblock_type memory;          // 「可用内存」表
    struct memblock_type reserved;        // 「已保留」表
    phys_addr_t current_limit;            // 分配上限
    bool bottom_up;                       // 分配方向（自低向高）
};
```

**核心设计**：内存用**两表模型**描述——`memory`（哪些物理区间可用）+ `reserved`（哪些被占用）。「可用」= `memory` 减去 `reserved` 的差集。

## 2. 核心 API：加区间与保留（mm/memblock.c）

| 函数 | 行号 | 作用 |
|------|------|------|
| `memblock_add` | :727 | 向 `memory` 表加一段可用区间 |
| `memblock_add_node` | :705 | 带 NUMA 节点的版本 |
| `memblock_reserve` | :871 | 向 `reserved` 表标记占用（保护内核/initrd/DTS） |
| `memblock_phys_alloc` | :406 | 分配物理地址（返回 phys_addr_t） |
| `memblock_alloc_try_nid` | :1631 | 分配并返回虚址（清零 + 按节点） |

**走读要点**：`memblock_add` 和 `memblock_reserve` 都收敛到内部 `memblock_add_range`——把新区间插入数组，**相邻则合并**、重叠则拆。这是「区间表」相对「位图」的优势：内存碎片少时数组很短。

## 3. 退役移交 Buddy：`memblock_free_all`（:2174）

```
memblock_free_all()                       // memblock.c:2174
        │
        ▼
free_low_memory_core_early()              // memblock.c:2126
        │  遍历 memory 表的每个区间
        ▼
__free_pages_core(...)                    // 逐页交给 Buddy（page_alloc）
```

**走读要点**：这是「启动分配器 → 运行时分配器」的**移交点**。移交后 memblock 不再分配，`page_alloc.c` 接管。Ch5 §4 讲的「memblock 退役四步」就是这里。

## 4. 为什么 bootmem 被 memblock 取代

| 维度 | bootmem（已删） | memblock（v6.6） |
|------|-----------------|------------------|
| 数据结构 | 位图（每页 1 bit） | 区间表（base+size） |
| 内存开销 | 与总内存成正比 | 与区间数成正比（通常远小） |
| 分配粒度 | 页 | 任意大小（向下对齐） |
| 删除版本 | v4.20（Mike Rapoport 主导） | 现存 |

**位图的致命伤**：物理内存越大、位图越大，且「查找连续空闲页」要扫描位图。区间表天然表达「连续区间」，启动阶段内存分布规整时优势明显。

---

## 与正文对应

| 附录内容 | 正文落点 |
|----------|----------|
| 三结构体字段 | Ch5 §1（memblock.h:59-98 字段级） |
| add/reserve 两表模型 | Ch5 §2（`/proc/iomem` 观察法） |
| `memblock_free_all` 移交 | Ch5 §4（四步退役） |
| bootmem 删除时间线 | Ch5 §5（v4.20 终结） |

---

## HFT / 嵌入式关联

**嵌入式启动优化**：memblock 的区间表在「内存规整」的嵌入式板子上极短，`memblock_add` 的合并逻辑几乎零开销；但若 DTS 碎片化严重，`reserved` 表会膨胀，启动时 `memblock_alloc` 的线性扫描变慢——排查启动慢时可先看 `/proc/iomem` 的 reserved 片段数。

---

## 相关章节

- 上一章：[appendix-D-进程地址空间.md](./appendix-D-进程地址空间.md)
- 下一章：[appendix-F-物理页分配.md](./appendix-F-物理页分配.md)

---

<details>
<summary>自测 5 问（点开看答案）</summary>

**Q1：memblock 用哪两个表描述内存？「可用」怎么算？**

`memory`（可用区间）+ `reserved`（占用区间）两表；「可用」= memory 减 reserved 的差集。

**Q2：`memblock_add` 和 `memblock_reserve` 内部收敛到哪个函数？做了什么？**

都收敛到 `memblock_add_range`，把新区间插入数组，相邻合并、重叠拆。

**Q3：`memblock_free_all` 的移交点在哪？**

`memblock_free_all`（:2174）→ `free_low_memory_core_early`（:2126）→ `__free_pages_core` 逐页交给 Buddy，之后 page_alloc 接管。

**Q4：bootmem 位图相对 memblock 区间表的致命伤是什么？**

位图大小与总内存成正比，且查找连续空闲页要扫描位图；区间表大小与区间数成正比（通常远小），天然表达连续性。

**Q5：bootmem 在哪一版被删除？谁主导？**

v4.20，Mike Rapoport 主导（Ch5 §5）。

</details>
