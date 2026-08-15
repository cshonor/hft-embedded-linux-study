# Bootlin: 内存监控工具

> **来源:** [Bootlin Kernel Training — Memory Management](https://bootlin.com/docs/kernel/)
> **主题:** /proc/meminfo, buddyinfo, slabinfo, vmstat, DAMON
> **对标旧书:** ULK3 Appendix (部分 proc 接口)

---

## 讲义要点

### /proc/meminfo

```bash
cat /proc/meminfo
# MemTotal:       16384000 kB    — 总物理内存
# MemFree:          123456 kB    — 空闲内存
# MemAvailable:    8123456 kB    — 可用内存 (含可回收缓存)
# Buffers:          234567 kB    — buffer cache
# Cached:          5678901 kB    — page cache
# SwapCached:          123 kB    — swap 中缓存的页
# Active:          4567890 kB    — active LRU
# Inactive:        3456789 kB    — inactive LRU
# Slab:             567890 kB    — slab 分配器
# SReclaimable:     234567 kB    — 可回收 slab
# SUnreclaim:       333323 kB    — 不可回收 slab
# AnonPages:       2345678 kB    — 匿名页
# Mapped:           123456 kB    — 映射到页表
# Shmem:             56789 kB    — 共享内存
# KReclaimable:     234567 kB    — 内核可回收
# HugePages_Total:    1024       — 大页总数
# HugePages_Free:      512       — 空闲大页
# Hugepagesize:      2048 kB     — 大页大小 (2MB)
```

### /proc/buddyinfo

```bash
cat /proc/buddyinfo
# Node 0, Zone Normal, type    Unmovable  movable  reclaimable
#   free 3985 234 56 12 3 1 0 0 0 0 0
#   ↑    ↑   ↑  ↑  ↑  ↑ ↑ ↑ ↑ ↑ ↑ ↑
#   node zone    迁移类型    order 0-10 空闲块数
#   order 0 = 4KB, order 1 = 8KB, ..., order 10 = 4MB
```

### /proc/slabinfo / slabtop

```bash
# slab 统计
cat /proc/slabinfo | head -5
# name     <active_objs> <num_objs> <objsize> <objperslab> <pagesperslab>
# inflight_0       0        0          16         255             1
# kmalloc-8     12345   13056           8         512             1

# slabtop (按用量排序)
slabtop -o -s c | head -15
```

### vmstat / sar

```bash
# 页回收统计
cat /proc/vmstat | grep -E "pgscan|pgsteal|pgalloc"
# pgalloc_dma 0
# pgalloc_normal 12345678
# pgalloc_movable 0
# pgscan_anon 123456          — 扫描的 anon 页
# pgscan_file 234567          — 扫描的 file 页
# pgsteal_anon 12345          — 回收的 anon 页
# pgsteal_file 234567         — 回收的 file 页

# sar 页回收监控
sar -B 1 5  # 每秒, 共5次
# pgpgin/s pgpgout/s fault/s majflt/s pgfree/s pgscank/s pgscand/s pgsteal/s
```

### DAMON

```bash
# 5.15+ 启用 DAMON
echo <pid> > /sys/kernel/debug/damon/target_ids
echo "5 100 1000 10 1000" > /sys/kernel/debug/damon/attrs
echo on > /sys/kernel/debug/damon/monitor_on
# 查看结果需要 CONFIG_DAMON_VADDR_KUNIT_TEST 或 debugfs
```

---

## 动手实验

```bash
# 1. 全面内存诊断
echo "=== meminfo ===" && head -20 /proc/meminfo
echo "=== buddyinfo ===" && cat /proc/buddyinfo
echo "=== slabtop ===" && slabtop -o -s c | head -10
echo "=== vmstat ===" && cat /proc/vmstat | grep -E "pgscan|pgsteal"
echo "=== hugepages ===" && cat /proc/meminfo | grep -i huge

# 2. 进程级内存
cat /proc/<pid>/status | grep -i vm
# VmPeak: 123456 kB    — 峰值虚拟内存
# VmSize:  98765 kB    — 当前虚拟内存
# VmRSS:   45678 kB    — 物理内存
# VmHWM:   56789 kB    — 峰值物理内存

# 3. 进程 smaps
cat /proc/<pid>/smaps_rollup
# Rss:    45678 kB
# Pss:    41234 kB    — Proportional Set Size (按共享比例分摊)
# Uss:    34567 kB    — Unique Set Size (独占)
```

---

## 与旧书差异

| ULK3 | Bootlin 讲义 |
|------|-------------|
| 部分 /proc 接口 | 丰富 + DAMON (5.15+) |
| 无 Pss/Uss | smaps_rollup 提供 |
| 无 PSI | /proc/pressure/ |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** MemAvailable 和 MemFree 的区别？为什么 HFT 监控应该看 MemAvailable？

> MemFree 是完全空闲的物理内存。MemAvailable = MemFree + 可回收缓存（clean page cache + SReclaimable slab）- 共享内存。MemAvailable 更准确反映"实际可用"内存。HFT 应监控 MemAvailable——如果 MemFree 很低但 MemAvailable 充足，说明缓存占用正常，系统不会触发回收。如果 MemAvailable 也低，才需要担心。

**Q2:** buddyinfo 中 order 9 (2MB) 空闲块为 0 意味着什么？对 HFT 有什么影响？

> order 9 空闲块为 0 意味着没有连续的 2MB 物理页可供分配。HFT 需要大页（2MB）减少 TLB miss，如果运行时申请大页失败会退化到 4KB 页，TLB miss 增加。解决：(1) 启动时预留 `echo 1024 > /proc/sys/vm/nr_hugepages`；(2) 禁用 THP（避免 khugepaged 零散消耗）；(3) 用 `mmap(MAP_HUGETLB)` 预分配。

</details>
