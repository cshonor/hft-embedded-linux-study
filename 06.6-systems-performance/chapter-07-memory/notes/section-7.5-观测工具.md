## 7.5 观测工具

> 章节导航：[7.4 分析方法论](./section-7.4-分析方法论.md) · 上一篇 ← · [本章导读](../README.md)

**本节讲什么**：内存观测的四层工具（全局统计 / 进程映射 / slab-NUMA / perf-BPF 专项）、VSZ/RSS/PSS 三指标精读、缺页火焰图与 drsnoop 的用法、工具选型速查。

### 要点

| # | 要点 | 一句话 |
|---|------|--------|
| 1 | `vmstat` 的 **si/so** 是 HFT 红线列 | 持续非零 = swap |
| 2 | **PSS 是公平口径** | RSS 共享页重复算 |
| 3 | `pmap -X` + smaps 是**映射归因** | 哪段映射占内存 |
| 4 | 缺页火焰图回答**谁在 touch 新页** | leak 与工作集漂移 |
| 5 | drsnoop 抓 **direct reclaim 受害者** | 分配尖刺定罪 |

---

### 一、全局统计层

| 工具 | 看什么 | 关键字段 |
|------|--------|----------|
| **`vmstat 1`** | 全局内存与 Swap | `free`、`buff/cache`、**`si`/`so`**、`swpd` |
| **`sar -r` / `sar -B`** | 历史内存、分页 | `-B`：pgscan/pgsteal（回收）、majflt/s |
| **`/proc/meminfo`** | 权威细分 | MemAvailable、AnonPages、Slab、SwapCached |

**vmstat 精读**：

```
procs -----------memory---------- ---swap-- -----io---- -system-- ------cpu-----
 r  b   swpd   free   buff  cache   si   so    bi    bo   in   cs us sy id wa st
 2  0  52428 1204332 482112 83920132    0    0     0    12  ...        ...
     └──────────┬──────────┘    └─┬─┘ └─┬─┘
       swpd=已换出 KB         si=换入  so=换出（KB/s）
```

- **si/so 持续非零 = swap 在发生**——HFT 灾难信号（一次 swap-in 是 ms 级盘读）
- `b` 列（阻塞进程）高 + so 高 = 有进程在等换页
- swpd 大但 si/so=0：历史换出、现在不活跃——不算当前压力，但说明曾经压力过

**/proc/meminfo 的关键行**：

```
MemAvailable:    88123456 kB   ← 真正可用（free + 可回收 - 保留水位）
AnonPages:       38221044 kB   ← 匿名页（进程堆栈——swap 候选）
Slab:             4210332 kB   ← 内核对象缓存（slabtop 细分）
SwapCached:        512344 kB   ← 换出又被读回的页（还在 swap 区）
```

### 二、进程映射层

| 工具 | 看什么 |
|------|--------|
| **`top` / `ps`** | **VSZ**（虚拟）vs **RSS**（常驻物理） |
| **`pmap -x` / `pmap -X`** | 映射明细；`-X` 额外给 **PSS** 分摊 |
| **`/proc/PID/smaps`** | 每映射的 Rss/Pss/Shared/Private——脚本化分析 |

**三指标精读**：

| 指标 | 含义 | 陷阱 |
|------|------|------|
| **VSZ** | 地址空间大小——含未 touch 的映射 | malloc 保留区/稀疏映射让它**远大于 RAM**，本身无害 |
| **RSS** | 实际在物理内存的页 | **共享库整页算给每个进程**——100 个进程共享 libc 时总 RSS 重复计费 |
| **PSS** | 共享页按进程数分摊 | 总和 = 全系统真实占用（容器计费口径） |

**pmap -X 精读**（归因哪段映射在吃内存）：

```
Address  Perm   Offset Device   Inode  Size  Rss  Pss Shared_Private...  Mapping
00007f00 rw-    0000000  00:00        0  1024  512  512         0        [heap]     ← 堆泄漏看这
00007f12 rw-    ...                        512  256   64       192        libc 的数据页 ← 共享分摊
00007f2a rw-s   ...                   1048576 1048576 1048576  0        /dev/hugepages/... ← hugepage 映射
```

heap 段持续涨 = 用户态泄漏；映射数持续涨 = mmap 泄漏（没 munmap）；anon 段涨 = 线程栈泄漏（线程没回收）。

### 三、内核与 NUMA 层

| 工具 | 用途 |
|------|------|
| **`slabtop`** | 内核 slab 各 cache 占用排行（dentry/inode/skbuf——slab 机制见 [06-linux-mm ch08](../../../06-linux-mm/chapter-08-slab-allocator/)） |
| **`numastat`** | numa_hit（本地命中）vs numa_foreign（远端访问） |
| `cat /proc/zoneinfo` | 各 zone 水位（min/low/high）与剩余 |

**numastat 判读**：numa_foreign 占比高 = 跨 NUMA 访问多——内存带宽减半 + 延迟上升；对策是绑核绑内存同 node（`numactl --cpunodebind=0 --membind=0`）。NUMA 架构与 buddy 的 zone 机制见 [06-linux-mm ch06](../../../06-linux-mm/chapter-06-physical-page-allocation/)。

### 四、perf 与 BPF 专项

| 工具 | 用途 |
|------|------|
| **`perf stat -e page-faults,major-faults,minor-faults`** | 缺页计数（分 minor/major） |
| **`perf record -e page-faults -g`** | 缺页火焰图——谁在 touch 新页 |
| **`drsnoop`（BCC）** | **direct reclaim** 逐次延迟（谁在等回收） |
| **`wss`（BCC，referenced 位）** | 进程 WSS 估算 |

```bash
# Swap 是否在发生（持续监控）
vmstat 1 | awk 'NR>2 {print $7,$8}'   # si so

# 缺页热点（开发/压测环境）
perf record -e major-faults -g -p $(pidof strategy) -- sleep 30
perf script | stackcollapse-perf.pl | flamegraph.pl > major-fault.svg

# direct reclaim 受害者逐次输出
sudo drsnoop-bpfcc
# TIME(s)  COMM   PID    LAT(ms)
# 1.234    strategy 4321  3.5       ← 分配路径等回收 3.5ms
```

**drsnoop 的判读**：LAT(ms) 列就是分配尖刺的内存侧证据——配合 [7.4 传导链](./section-7.4-分析方法论.md) 定位是水位问题还是回收效率问题。缺页事件机制（fault 四路分发）见 [06-linux-mm ch04](../../../06-linux-mm/chapter-04-process-address-space/)——perf 抓的 page-faults 就是 do_anonymous_page/wp_page_copy 这些路径的触发。

### 五、工具选型速查

| 问题 | 第一工具 | 深挖 |
|------|---------|------|
| 现在有内存压力吗？ | PSI + vmstat si/so | sar -B 回收历史 |
| 真实可用多少？ | free 的 available | /proc/meminfo |
| 哪个进程占内存？ | top/ps RSS | pmap -X（PSS 归因） |
| 是泄漏吗？ | RSS 日曲线形状 | pmap -X 分项 + 缺页火焰图 |
| 谁在等回收？ | drsnoop | zoneinfo 水位 |
| NUMA 均衡吗？ | numastat | numactl 调整 |
| 内核内存去哪了？ | slabtop | /proc/slabinfo |

### HFT / 嵌入式关联

- **巡检常驻**：vmstat si/so（恒零红线）+ PSI memory + MemAvailable——低开销三件。
- **日曲线**：RSS/PSS/slab 三条曲线进监控——斜率告警早于阈值告警。
- **每次上线前**：major-faults 计数应恒零（mlock 生效验证）；drsnoop 短窗口确认无 direct reclaim。
- **嵌入式**：/proc/meminfo 是全部观测面（无 BCC）——Slab 与 MemAvailable 的比值是内核开销的健康指标。

### 衔接

- 上一节：[7.4 分析方法论](./section-7.4-分析方法论.md)
- 关联：[Ch13 perf](../../chapter-13-perf/)、[Ch15 BPF](../../chapter-15-bpf/)、[附录 C](../../appendix-C-bpftrace单行命令.md)、[06-linux-mm ch08 slab](../../../06-linux-mm/chapter-08-slab-allocator/)、[06-linux-mm ch04 fault](../../../06-linux-mm/chapter-04-process-address-space/)、[06-linux-mm ch06 zone/水位](../../../06-linux-mm/chapter-06-physical-page-allocation/)

---

### 常见陷阱

1. **vmstat 只看 free 列**——si/so 才是红线；free 低是 cache 占用的正常形态。
2. **slabtop 不看**——内核对象（dentry/skbuf）膨胀挤占用户内存，进程视角不可见。
3. **pmap 不用 -X**——-x 只有 RSS；-X 的 PSS/私有分项才是归因依据。
4. **VSZ 大当泄漏**——稀疏映射/malloc 保留区让 VSZ 天然大；看 RSS/PSS 和映射明细。
5. **缺页火焰图在污染态跑**——应在压测/复现场景抓，正常巡检抓不到东西。

<details>
<summary>自测题（点击展开）</summary>

1. vmstat 中哪些列对 HFT 最关键？
   <details><summary>答</summary>si/so（swap in/out）——持续非零 = 匿名页换页在发生 = ms 级停顿源，不可接受。</details>
2. RSS 与 PSS 的计算差异？
   <details><summary>答</summary>RSS 把共享库整页算给每个进程（重复计费）；PSS 把共享页按共享进程数分摊——全系统 PSS 之和 = 真实总占用。</details>
3. pmap -X 里 heap 涨、映射数涨、anon 段涨各指什么泄漏？
   <details><summary>答</summary>heap 涨 = malloc 泄漏；映射数涨 = mmap 没 munmap；anon 涨（各 8MB 级）= 线程栈泄漏（线程没回收）。</details>
4. drsnoop 的 LAT 列是什么的证据？
   <details><summary>答</summary>分配路径同步等回收的毫秒数——分配尖刺（malloc/缺页变慢）的内存侧直接定罪。</details>
5. numa_foreign 高说明什么、怎么修？
   <details><summary>答</summary>跨 NUMA 访问占比高——带宽减半延迟上升；numactl 把进程的 CPU 与内存绑到同一 node。</details>

</details>


---

← [本章导读](../README.md)
