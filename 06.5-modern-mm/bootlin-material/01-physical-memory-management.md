# Bootlin: 物理内存管理 (zone / buddy / pcp)

> **来源:** [Bootlin Kernel Training — Memory Management](https://bootlin.com/docs/kernel/)
> **主题:** 物理内存管理：zone、buddy 系统、per-CPU 页缓存
> **对标旧书:** ULK3 Ch8 / LKD3 Ch12 / Mel Gorman《Understanding the Linux VM Manager》

---

## 讲义要点

### 内存 zone

```
物理内存按用途划分为 zone:
  ZONE_DMA    — DMA 可用区域 (x86: <16MB, ARM64: 通常无此 zone)
  ZONE_DMA32  — 32-bit DMA 可用 (x86: <4GB)
  ZONE_NORMAL — 正常地址区域 (直接映射)
  ZONE_HIGHMEM— 高端内存 (32-bit 系统, 64-bit 无此 zone)
  ZONE_MOVABLE— 可移动页 (大页预留/内存热插拔)
```

### 伙伴系统

```c
// 源码路径: mm/page_alloc.c
struct zone {
    struct free_area free_area[MAX_ORDER + 1];  // order 0-10
    // ...
};

struct free_area {
    struct list_head free_list[MIGRATE_TYPES];  // 按迁移类型分组
    unsigned long nr_free;
};

// 分配: alloc_pages(gfp, order)
// 1. 查 free_area[order]，有空闲则取
// 2. 无空闲，查 free_area[order+1]，分裂
// 3. 直到 free_area[MAX_ORDER]，OOM

// 释放: __free_pages(page, order)
// 1. 检查 buddy 是否空闲
// 2. 合并为 order+1 块
// 3. 递归直到无法合并
```

### Per-CPU 页缓存 (pcp)

```c
// 快速分配路径: 不需要 zone->lock
struct per_cpu_pageset {
    struct per_cpu_pages pcp;  // per-CPU 页缓存
};

struct per_cpu_pages {
    struct list_head lists[MIGRATE_PCPTYPES];  // 3 种迁移类型
    int count;      // 当前缓存页数
    int high;       // 高水位 (超过则归还 zone)
    int batch;      // 批量操作大小
};

// alloc_pages 快路径:
// 1. 从 pcp 取页 (无锁)
// 2. pcp 空则从 zone 批量补充 batch 页到 pcp
// 3. 补充需要 zone->lock
```

### 迁移类型 (Migrate Types)

```c
enum migratetype {
    MIGRATE_UNMOVABLE,   // 不可移动 (内核分配)
    MIGRATE_MOVABLE,     // 可移动 (用户页, 页缓存)
    MIGRATE_RECLAIMABLE, // 可回收 (slab)
    MIGRATE_PCPTYPES,    // pcp 类型数
    MIGRATE_HIGHATOMIC,  // 高阶原子分配
    MIGRATE_CMA,         // CMA 连续内存分配
    MIGRATE_ISOLATE,     // 隔离 (内存热插拔)
};
```

---

## 动手实验

```bash
# 1. 查看 zone 信息
cat /proc/zoneinfo | head -50

# 2. 查看 buddy 信息
cat /proc/buddyinfo
# Node 0, Zone Normal, type    Unmovable  movable  reclaimable
#   free 3985 234 56 12 3 1 0 0 0 0 0  # order 0-11 的空闲块数

# 3. 查看内存布局
dmesg | grep -i "zone\|memory"
# 或
numactl --hardware

# 4. 观察 alloc_pages 失败
echo 1 > /proc/sys/vm/panic_on_oom
# (危险! 仅在测试环境)
```

---

## 与旧书差异

| Mel Gorman / ULK3 | Bootlin 讲义 |
|-------------------|-------------|
| 无 per-CPU pcp 细节 | pcp 是快路径关键 |
| 无迁移类型 | 迁移类型用于反碎片 |
| ZONE_HIGHMEM 重点 | 64-bit 无 HIGHMEM |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** per-CPU 页缓存 (pcp) 如何减少锁竞争？

> 伙伴系统的 `zone->lock` 是全局锁。pcp 让每个 CPU 维护一个小页池（默认 batch=63 页），分配时优先从 pcp 取（无锁）。只有 pcp 空时才获取 `zone->lock` 批量补充。多核系统上 pcp 减少 95%+ 的 zone lock 获取次数。

**Q2:** 伙伴系统的 MAX_ORDER 是多少？为什么有上限？

> MAX_ORDER 通常为 10（即 2^10 = 4MB 连续页）。上限原因：(1) 更大的连续物理块极难找到（碎片化）；(2) 分裂/合并递归深度限制；(3) 历史原因。需要更大连续内存用 `alloc_contig_range()` 或大页（2MB=order 9, 1GB=order 18 via hugetlb）。

</details>
