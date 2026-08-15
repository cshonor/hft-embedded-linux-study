# Chapter 02: SLAB/SLUB 分配器

> 来源：Bootlin（概述）+ LWN（SLUB 细节 + SLAB 移除）
> 对标：Mel Gorman Ch3（SLAB → SLUB）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [slab-slub-overview](notes/01-slab-slub-overview.md) | Bootlin：SLAB/SLUB 架构、per-CPU partial、kmalloc |
| 2 | [slub-allocator](notes/02-slub-allocator.md) | LWN：SLUB 设计原理、fast/slow path、cmpxchg 无锁分配 |
| 3 | [slub-vs-slab](notes/03-slub-vs-slab.md) | LWN：SLUB vs SLAB 对比、内存开销、NUMA 支持 |
| 4 | [slab-removal](notes/04-slab-removal.md) | LWN：SLAB 移除历程、6.x 内核 SLUB 为唯一实现 |

## HFT 关联

- **对象池**：SLUB 的 per-CPU freelist 实现 O(1) 无锁分配，HFT 可用 `kmem_cache_create` 定制对象池
- **cmpxchg 快路径**：SLUB 快路径仅一条 cmpxchg 指令，延迟 < 50ns
- **NUMA locality**：SLUB 保证 per-CPU slab 来自本地 NUMA 节点，减少跨节点访问延迟
- **SLAB 移除**：6.x 内核不再有 SLAB，所有旧代码必须迁移到 SLUB API

## 交叉引用

- `06-linux-mm/`：Mel Gorman Ch3（SLAB 实现，已过时）
- `05.5-modern-kernel/chapter-04-synchronization/`：cmpxchg 与无锁数据结构
