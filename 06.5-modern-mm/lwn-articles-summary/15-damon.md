# DAMON (Data Access MONitor)

> **原文:** [DAMON: Data access monitoring](https://lwn.net/Articles/812704/) (LWN, 2019)
> **作者:** SeongJae Park
> **内核版本:** 5.15+
> **对标旧书:** 无 (ULK3/LKD3 未涉及)

---

## 核心观点

DAMON 是内核内建的数据访问监控框架，以低开销跟踪内存访问模式。

### 传统监控的问题

- `/proc/pid/smaps`：精确但开销极大（遍历所有 VMA + 页表）
- `perf mem`：采样精确但需要硬件支持 + 高开销
- `mincore()`：只告诉你页是否在内存，不告诉你是否被访问

### DAMON 设计

```
DAMON 架构:
  ┌──────────┐     ┌──────────────┐     ┌───────────┐
  │ Target   │────→│ Monitoring   │────→│ Results   │
  │ (进程/   │     │ Attributes   │     │ (访问模式) │
  │  地址空间)│     │ (采样间隔等)  │     │           │
  └──────────┘     └──────────────┘     └───────────┘
                         │
                    ┌────┴────┐
                    │ Regions │  (自适应划分地址空间)
                    └─────────┘
```

**核心思想：** 不监控每一个页（开销大），而是将地址空间划分为少量"区域"（region），对每个区域采样少量页，推断整个区域的访问模式。区域边界自适应调整（类似 adaptive sampling）。

### DAMON API

```bash
# 启用 DAMON (5.15+)
# 需要编译 CONFIG_DAMON=y

# 通过 debugfs 控制
echo <pid> > /sys/kernel/debug/damon/target_ids
echo "5 100 1000 10 1000" > /sys/kernel/debug/damon/attrs
# 5: 采样间隔 (ms)
# 100: 聚合间隔 (ms)
# 1000: 更新间隔 (ms)
# 10: 最小区域数
# 1000: 最大区域数

# 启动监控
echo on > /sys/kernel/debug/damon/monitor_on

# 查看结果
cat /sys/kernel/debug/damon/target_ids  # 查看目标
# 结果通过 DAMON DAMOS (action) 或 tracepoint 获取
```

### DAMOS (DAMon Operation Scheme)

```c
// 源码路径: mm/damon/dbgfs.c
// DAMOS: 基于 DAMON 监控结果自动执行操作

// 示例: 对 100ms 未访问的页执行 reclaim
struct damos scheme = {
    .min_sz_region = PAGE_SIZE,      // 最小区域大小
    .max_sz_region = ULONG_MAX,
    .min_access_rate = 0,            // 访问率 0%
    .max_access_rate = 0,            // = 未访问
    .action = DAMOS_PAGEOUT,         // 回收
};
```

### 性能开销

| 配置 | 开销 |
|------|------|
| 10 区域, 5ms 采样 | <1% CPU |
| 100 区域, 1ms 采样 | ~3% CPU |
| 全精度 (mincore 对比) | >50% CPU |

---

## 与旧书差异

| ULK3 / LKD3 | 现代实现 |
|-------------|---------|
| 无数据访问监控 | DAMON (5.15+) |
| `/proc/pid/smaps` 高开销 | DAMON 低开销采样 |
| 无自动操作 | DAMOS 可自动回收/proactive compaction |

---

## HFT 关联

DAMON 可用于 HFT 系统运维：(1) 监控交易进程的内存访问模式，识别冷热区域；(2) 配合大页管理，将热数据集中到大页；(3) 但 HFT 不应启用 DAMOS 自动回收（可能误回收交易页）。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** DAMON 如何实现低开销？

> DAMON 不监控每个页，而是将地址空间划分为少量区域（默认 10 个），每个区域只采样少量页。区域边界根据访问模式自适应调整——频繁访问的区域被细分，统一访问模式的区域被合并。总采样页数远少于全部页数，开销 <1% CPU。

**Q2:** DAMON 的 "region" 概念和 VMA 有什么区别？

> VMA 是进程虚拟地址空间的逻辑分区（按 mmap/munmap 划分）。DAMON region 是监控用的采样分组，可以跨 VMA 或在 VMA 内部进一步细分。DAMON 根据 region 内页的访问模式自适应调整边界，与 VMA 无关。一个 VMA 可能包含多个 DAMON region。

</details>
