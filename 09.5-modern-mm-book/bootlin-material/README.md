# Bootlin 训练材料要点 — 内存管理

> Bootlin 公开培训讲义摘要 + 实验操作清单（MM 专项）。
> 来源: https://bootlin.com/docs/

## 主要课程

### 内存管理子系统
- [ ] 物理内存管理（zone / buddy / pcp）
- [ ] Slab/SLUB 分配器
- [ ] vmalloc 与非连续内存
- [ ] 页表与 TLB 管理
- [ ] 页缓存与 folio API
- [ ] 页回收（MGLRU / swap / zswap）
- [ ] OOM killer 与 PSI
- [ ] 内存 cgroup

### 实验操作
- [ ] `/proc/meminfo` / `/proc/buddyinfo` 解读
- [ ] `/proc/slabinfo` / `slabtop` 工具
- [ ] `vmstat` / `sar -B` 页回收监控
- [ ] DAMON 数据访问监控实验
- [ ] cgroup v2 内存限制实验

> 每节课整理：讲义要点 + 动手实验步骤 + 与 Mel Gorman 旧书差异
