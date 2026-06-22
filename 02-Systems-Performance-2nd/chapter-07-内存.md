# Ch 7 内存 · Memory

> **Systems Performance 2nd** · Brendan Gregg · **精读**

> 本章定位：**主存耗尽并开始换页时，内存会成为最严重的瓶颈之一** — Ch 6 的低 IPC / stall 往往指向这里。本章从虚拟内存、缺页、Swap、NUMA、TLB/大页到 Slab/用户态分配器，给出 **内存资源层的概念 → 分析 → 工具 → 调优** 全链路。

---

## 大白话 · 本章就五件事

> **内存问题分两种：正常的 cache 未命中，和要命的 Swap/OOM。**

**① 虚拟内存是抽象，缺页才是「真花钱」。**

- 每个进程看到巨大线性地址空间；物理页 **按需映射** — 首次访问触发 **page fault**，内核才分配真页。
- **文件换页**（页缓存）通常可接受；**匿名 Swap**（堆栈）是性能杀手 — HFT 裸机应 **尽量零 Swap**。

**② WSS 决定你「需要多少真内存」。**

- **Working Set Size** = 进程活跃访问的页面集；装进 cache 最快，超出主存 → Swap 地狱。
- 区分 **内存泄漏** vs **正常增长**（预热缓存）— 要靠分配追踪，不能只看 RSS 曲线。

**③ 硬件：UMA/NUMA、MMU、TLB、大页。**

- **NUMA**：本地节点快、远程节点慢 — 线程与内存必须 **同节点**。
- **TLB miss** 贵 — **Huge Pages**（2MB/1GB）减 miss；DPDK / 大堆 Java 都相关。

**④ 内核释放内存有顺序 — 直到 OOM Killer。**

- Free list → 回收页缓存（`swappiness`）→ kswapd → **direct reclaim**（拖慢当前线程）→ **OOM**。
- **PSI memory** 比 `free` 更能反映「等内存」的压力。

**⑤ 工具 + 调优：vmstat/si/so、perf 缺页火焰图、drsnoop、numactl。**

- `vmstat` 的 **si/so** 看 Swap；`pmap -X` 看 **PSS**；BPF **`drsnoop`** 抓 direct reclaim 延迟。
- 调优：`swappiness=1`、大页、`LD_PRELOAD` TCMalloc/jemalloc、cgroups 限额。

下面按原书 7.1–7.6 展开。

---

## 7.1–7.2 内存核心概念

### 虚拟内存（Virtual Memory）

| 作用 | 说明 |
|------|------|
| **抽象** | 进程看到私有、连续（逻辑上）的地址空间 |
| **隔离** | 进程 A 不能踩进程 B 的页 |
| **多任务** | 物理内存有限，虚拟空间可远大于 RAM |
| **Overcommit** | 内核允许承诺超过物理内存的映射 — **OOM 风险** |

**HFT：** 策略进程 + order book + 页缓存 + 监控 — 要算 **真实 RSS/PSS**，不能假设「还有 free 就安全」。

→ [01-CSAPP Ch9](../01-CSAPP-3rd/chapter-09-虚拟内存.md) · [06-Gorman](../06-Linux-Virtual-Memory-Manager/)

### 按需分页与缺页异常

```
进程访问虚拟地址
    ↓
页表项无效 / 未 present？
    ↓ 是
Page Fault（缺页异常）
    ↓
内核：分配物理页 / 读入文件页 / COW / Swap-in
    ↓
返回用户态继续执行
```

| 缺页类型 | 含义 | 性能 |
|----------|------|------|
| **Minor fault** | 页已在内存，仅更新页表（如 COW、首次 touch 已分配页） | 相对轻 |
| **Major fault** | 需 I/O：读文件页或 **Swap-in** | **重** — 微秒～毫秒级 |

**HFT：** 热路径上 unexpected **major fault** = 延迟尖刺；启动后应 **预热（touch）** 关键数据结构，或启动时 mlock。

### 换页 vs 交换（Paging vs Swapping）

| 术语 | Linux 语境 | 好坏 |
|------|------------|------|
| **File system paging** | 文件映射页在 **page cache** 中换入换出 | 通常可接受（读 mmap 文件等） |
| **Anonymous paging** | 堆、栈、匿名 mmap — **无文件后备** | Swap 到磁盘时 **极慢** |
| **Swapping** | Gregg/Linux 常 **特指匿名页换出到 swap 设备** | **坏** — HFT 裸机目标：si/so ≈ 0 |

```bash
vmstat 1
# si = swap in,  so = swap out  —  任一持续非 0 要立刻查
```

### 工作集大小（WSS）

**WSS** = 进程在一段时间内 **实际频繁访问** 的页面集合大小。

| WSS 相对资源 | 表现 |
|--------------|------|
| WSS ⊂ L3 cache | 最理想 — 与 Ch 6 IPC 高一致 |
| WSS ⊂ RAM | 正常 — 无 Swap |
| WSS > RAM | **Thrashing** — Swap 风暴，系统假死 |

**估算：** BPF 实验工具 `wss`、perf 缺页采样、或短期 `pmap`/RSS 观测 — 用于容量规划与 leak 排查。

**HFT：** order book 常驻结构 = WSS 主体；**预分配 + 池化** 让 WSS 稳定、可预测，避免运行期堆膨胀。

---

## 7.3 硬件与软件架构

### 硬件：DRAM、UMA、NUMA、MMU、TLB

| 组件 | 性能要点 |
|------|----------|
| **DRAM** | 容量大、延迟远高于 cache |
| **UMA** | 所有 CPU 访存延迟一致（老单机） |
| **NUMA** | 每 socket **本地内存** 快，**远程** 慢 1.5–3× 常见 |
| **MMU** | 虚拟 → 物理地址翻译 |
| **TLB** | 页表项缓存；**TLB miss** 触发页表 walk — 贵 |
| **Huge Pages** | 2MB / 1GB 页 → **同样映射范围 TLB 项更少** |

**NUMA 本地性（HFT 必做）：**

```bash
numactl --hardware          # 看节点与距离
numastat                    # 本地 vs 远程分配
numactl --cpunodebind=0 --membind=0 ./strategy
```

→ Ch 6 [绑核与 NUMA](./chapter-06-中央处理器.md#64-硬件与软件架构) · [04-Hennessy](../04-Computer-Architecture-6th/)

### Linux 释放内存机制（由轻到重）

```
① Free List（有空闲页直接用）
    ↓ 不足
② 回收 Page Cache（文件页，受 vm.swappiness 影响倾向）
    ↓ 仍不足
③ kswapd 后台扫描回收
    ↓ 仍不足
④ Direct Reclaim（在 fault/alloc 路径上同步回收 — 拖慢当前线程）
    ↓ 仍不足
⑤ OOM Killer（选进程杀）
```

| 阶段 | HFT 信号 |
|------|----------|
| **Direct reclaim** | 延迟毛刺、BPF `drsnoop` 有事件 |
| **Swap out (so)** | **不可接受** 于 tick 热路径机器 |
| **OOM** | 进程消失 — 比慢更糟 |

→ [06-Gorman ch13 内存耗尽](../06-Linux-Virtual-Memory-Manager/chapter-13-内存耗尽管理.md)

### 内存分配器

**内核：Slab / SLUB**

| 分配器 | 作用 |
|--------|------|
| **Slab / SLUB** | 对象缓存（dentry、inode、task_struct…）— 减少频繁 alloc_page |
| 查看 | `slabtop` — 哪个 cache 涨 |

**用户态：**

| 分配器 | 特点 | HFT |
|--------|------|-----|
| **glibc (ptmalloc/dlmalloc)** | 默认；多线程下 **arena 锁** 可能竞争 | 热路径少 malloc |
| **TCMalloc** | Google；per-thread cache，低锁竞争 | 可 `LD_PRELOAD` 对比 tail latency |
| **jemalloc** | 碎片控制好、arena 可配置 | 长期运行服务常用 |

**原则：** HFT tick 路径 **预分配 / 对象池 / 无分配** 优于换分配器；换分配器是 **第二道防线**。

→ [01-CSAPP Ch9 malloc](../01-CSAPP-3rd/chapter-09-虚拟内存.md) · Ch 5 [GC vs 手动管理](./chapter-05-应用程序.md#53-编程语言与垃圾回收)

---

## 7.4 分析方法论

### USE 方法（Memory）

| 字母 | 问什么 | 怎么量 |
|------|--------|--------|
| **U** Utilization | 物理/虚拟内存使用 | `free -h`、`/proc/meminfo`、RSS/PSS |
| **S** Saturation | 扫描、Swap、direct reclaim、OOM | `vmstat si/so`、`sar -B`、**PSI memory**、`dmesg` OOM |
| **E** Errors | 分配失败、ECC | `dmesg`、EDAC、应用 ENOMEM |

**PSI memory：**

```bash
cat /proc/pressure/memory
# some/full — 线程因等内存而 stall 的时间占比
```

→ [附录 A](./appendix-A-USE方法Linux.md) · Ch 6 [PSI 概念](./chapter-06-中央处理器.md#66-67-观测工具与可视化)

### 内存泄漏 vs 正常增长

| 现象 | 可能原因 | 验证 |
|------|----------|------|
| RSS 单调涨、从不回落 | **Leak** — alloc 无 free | Valgrind/ASan（测试）；生产 BPF uprobe malloc |
| 启动后涨然后平台 | 预热 cache、加载合约字典 | 预期行为 |
| PSS 涨、多进程共享库 | 映射增多 | `pmap -X` 分项 |

**HFT：** 7×24 运行的行情服务 — 画 **RSS/PSS 日曲线**；斜率异常先查 leak，再查 order book 是否无界增长。

### 缺页与 WSS 剖析

| 方法 | 工具 | 产出 |
|------|------|------|
| **Page fault profiling** | `perf record -e page-faults` | **缺页火焰图** — 谁在 touch 新页 |
| **Direct reclaim 延迟** | BPF `drsnoop` | 哪进程在等回收 |
| **WSS 估算** | BPF `wss`（实验） | 容量规划 |

---

## 7.5 观测工具

### 传统统计工具

| 工具 | 看什么 | 关键字段 |
|------|--------|----------|
| **`vmstat 1`** | 全局内存与 Swap | `free`、`buff/cache`、**`si`/`so`**、`swpd` |
| **`sar -r` / `sar -B`** | 历史内存、分页 | `-B`：pgpgin/out、fault |
| **`slabtop`** | 内核 Slab | 哪个 cache 占用异常 |
| **`numastat`** | NUMA 命中 | `numa_hit` vs `numa_foreign` |

### 进程级工具

| 工具 | 看什么 |
|------|--------|
| **`top` / `ps`** | **VSZ**（虚拟）vs **RSS**（常驻物理） |
| **`pmap -x` / `pmap -X`** | 映射明细；**PSS** = 共享页按比例分摊 |
| **`/proc/PID/smaps`** | 每映射 RSS/Pss/Shared — 脚本化分析 |

**VSZ vs RSS vs PSS：**

| 指标 | 含义 |
|------|------|
| **VSZ** | 地址空间大小 — 含未 touch 的映射，**可远大于 RAM** |
| **RSS** | 实际在物理内存的页 — 共享库 **整页算给每个进程** |
| **PSS** | 共享页按进程数分摊 — **更公平的总占用** |

### perf 与 BPF

| 工具 | 用途 |
|------|------|
| **`perf stat -e page-faults,major-faults,minor-faults`** | 缺页计数 |
| **`perf record -e page-faults -g`** | 缺页火焰图 |
| **`drsnoop`（BCC）** | 追踪 **direct reclaim** 路径延迟 |
| **`wss`（BCC，实验）** | 估算进程 WSS |

```bash
# Swap 是否在发生（持续监控）
vmstat 1 | awk 'NR>2 {print $7,$8}'   # si so

# 缺页热点（开发/压测环境）
perf record -e major-faults -g -p $(pidof strategy) -- sleep 30
perf script | stackcollapse-perf.pl | flamegraph.pl > major-fault.svg
```

→ [Ch 13 perf](./chapter-13-perf性能分析.md) · [Ch 15 BPF](./chapter-15-BPF技术.md) · [附录 C](./appendix-C-bpftrace单行命令.md)

---

## 7.6 调优指南

### 调优优先级（Gregg + HFT）

1. **消除不必要分配 / 控制 WSS**（Ch 5 应用层）
2. **NUMA 本地** — `numactl`、绑核与内存同节点（Ch 6）
3. **避免 Swap** — `swappiness`、足够 RAM、mlock 关键页
4. **大页** — 减 TLB miss
5. **分配器 / 脏页回写** — 按场景微调
6. **cgroups 限额** — 容器/多租户；裸机低延迟慎用硬限

### 关键 sysctl

| 参数 | 作用 | HFT 倾向 |
|------|------|----------|
| **`vm.swappiness`** | 0–100，倾向 swap 匿名页 vs 回收 file cache | **1–10**（裸机）；**0** 若保证 RAM 充足且接受 OOM 风险 |
| **`vm.min_free_kbytes`** | 保留最小空闲页 | 防突发 alloc 失败；过大浪费 RAM |
| **`vm.dirty_*`** | 脏页回写阈值 | 避免 burst write 拖慢；行情机日志异步 |
| **`vm.overcommit_memory`** | overcommit 策略 | 生产需理解 — 与 OOM 行为相关 |

**禁用 Swap（低延迟裸机常见）：**

```bash
swapoff -a    # 临时；/etc/fstab 去掉 swap 分区持久化
```

→ [11-HFT ch05](../11-HFT-Low-Latency-Practice/chapter-05-操作系统内核极致调优.md)

### 大页（Huge Pages）

| 类型 | 配置 | 用途 |
|------|------|------|
| **Transparent Huge Pages (THP)** | 内核自动合并 4KB→2MB | 方便但 **延迟不可预测** — HFT 常 **禁用或 madvise** |
| **Explicit Huge Pages** | `hugetlbfs` / `mmap(MAP_HUGETLB)` | DPDK、确定性延迟 |

→ [06-Gorman note-THP](../06-Linux-Virtual-Memory-Manager/note-透明大页THP.md) · [10-DPDK EAL](../10-DPDK-Low-Latency-Network/01-Intro-Book/notes/chapter-01-DPDK架构与EAL.md)

### 分配器与 NUMA

```bash
# 换 TCMalloc（benchmark 验证后再上生产）
LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libtcmalloc.so.4 ./strategy

# 进程绑到 node 0 的 CPU + 内存
numactl --cpunodebind=0 --membind=0 --preferred=0 ./strategy
```

### cgroups 内存控制

| 控制 | cgroup v2 示例 | 场景 |
|------|----------------|------|
| **硬限** | `memory.max` | 容器配额 |
| **swap 行为** | `memory.swap.max` | 限制 swap 使用 |
| **OOM 策略** | 组内 OOM 优先级 | 多服务混部 |

**HFT 共置：** 关键策略进程 **不要** 与未知内存占用的服务同 cgroup；OOM 杀错进程代价极高。

---

## 本章 Checklist

- [ ] 能解释 **minor vs major fault**、**file paging vs anonymous swap**
- [ ] 会用 **`vmstat` 的 si/so** 判断是否在 Swap
- [ ] 理解 **RSS vs PSS**，会用 `pmap -X` 看进程真实占用
- [ ] 对 NUMA 机器跑过 **`numastat`**，确认无大量 foreign 访问
- [ ] 知道 **direct reclaim** 与 **`drsnoop`** 的关系
- [ ] 裸机文档化：**swappiness、THP 策略、大页、是否 swapoff**

---

## HFT 精读捷径（Ch 7 在路线中的位置）

```
Ch 6  CPU — 低 IPC / cache-miss 高 → 跳本章
Ch 7  内存（本章：VM、Swap、NUMA、TLB、分配器）
  → Ch 8 文件系统（page cache 与 file paging 交叉）
  → Ch 6  绑核 + NUMA 一体调
  → 06-Gorman 内核 VM 深入
  → 10-DPDK 大页 / mempool
  → 11-HFT ch05 落地
```

**本章最小行动集：**

1. **`vmstat 1`** 看 60 秒 — 确认 **si/so = 0**（或解释为何非 0）。
2. **`numastat -p $(pidof strategy)`** — 本地 vs 远程页比例。
3. **`pmap -X $(pidof strategy) | tail -1`** — 记录 PSS 作为容量基线。
4. 压测一轮 **`perf stat -e major-faults,page-faults`** — 热路径应接近 0 major。

**Gregg 本章金句（HFT 版）：**

> **Swap 是内存饱和的尖叫** — `si/so` 非零比 `free` 还低更值得关注。  
> 低 IPC 时先问：**是 cache 布局问题，还是已经在换页了？**

---

## 相关章节

- 上一章：[chapter-06-中央处理器.md](./chapter-06-中央处理器.md)
- 下一章：[chapter-08-文件系统.md](./chapter-08-文件系统.md)
- 应用层分配：[chapter-05-应用程序.md](./chapter-05-应用程序.md)
- OS 虚拟内存：[chapter-03-操作系统.md](./chapter-03-操作系统.md)
- USE：[appendix-A-USE方法Linux.md](./appendix-A-USE方法Linux.md)
- 内核 VM 专书：[06-Linux-Virtual-Memory-Manager](../06-Linux-Virtual-Memory-Manager/)
- CSAPP：[01-CSAPP-3rd Ch9](../01-CSAPP-3rd/chapter-09-虚拟内存.md)
- HFT 调优：[11-HFT ch05](../11-HFT-Low-Latency-Practice/chapter-05-操作系统内核极致调优.md)
- 全书目录：[OUTLINE.md](./OUTLINE.md)
