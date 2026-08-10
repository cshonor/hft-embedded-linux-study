# Bootlin 训练材料要点 — 内存管理

> Bootlin 公开培训讲义摘要 + 实验操作清单（MM 专项）。
> 来源: https://bootlin.com/docs/
> 每节课整理：讲义要点 + 动手实验步骤 + 与 Mel Gorman 旧书差异 + 自测题

## 主要课程

### 内存管理子系统
- [x] [物理内存管理（zone / buddy / pcp）](01-physical-memory-management.md) — zone 划分、伙伴系统、per-CPU 页缓存
- [x] [Slab/SLUB 分配器](02-slab-slub-allocator.md) — per-CPU freelist、kmalloc cache 大小
- [x] [vmalloc 与非连续内存](03-vmalloc.md) — 虚拟连续物理不连续、地址空间布局
- [x] [页表与 TLB 管理](04-page-table-tlb.md) — 多级页表、ASID、大页、TLB shootdown
- [x] [页缓存与 folio API](05-page-cache-folio-api.md) — filemap_get_folio、readahead、writeback
- [x] [页回收（MGLRU / swap / zswap）](06-page-reclaim.md) — kswapd/direct reclaim、MGLRU、swap 配置
- [x] [OOM killer 与 PSI](07-oom-psi.md) — oom_badness、PSI 压力监控、poll 触发器
- [x] [内存 cgroup](08-memory-cgroup.md) — memory.max/high/min、per-cgroup LRU、OOM 隔离

### 实验操作
- [x] [监控工具（/proc/meminfo, buddyinfo, slabinfo, vmstat）](09-monitoring-tools.md) — 全面内存诊断、进程级 smaps
- [x] [DAMON + cgroup 实验](10-damon-cgroup-lab.md) — DAMON 监控实验、cgroup 限制实验、PSI+cgroup 联合实验

> 每节课整理：讲义要点 + 动手实验步骤 + 与 Mel Gorman 旧书差异 + 自测题
