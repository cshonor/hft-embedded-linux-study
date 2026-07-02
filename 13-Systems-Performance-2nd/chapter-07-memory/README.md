# Ch 7 内存 · Memory

> **Systems Performance 2nd** · Brendan Gregg · **精读**

> 本章定位：**主存耗尽并开始换页时，内存会成为最严重的瓶颈之一** — Ch 6 的低 IPC / stall 往往指向这里。本章从虚拟内存、缺页、Swap、NUMA、TLB/大页到 Slab/用户态分配器，给出 **内存资源层的概念 → 分析 → 工具 → 调优** 全链路。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 7.1–7.2 内存核心概念 | [notes/section-7.1-7.2-内存核心概念.md](./notes/section-7.1-7.2-内存核心概念.md) |
| 7.3 硬件与软件架构 | [notes/section-7.3-硬件与软件架构.md](./notes/section-7.3-硬件与软件架构.md) |
| 7.4 分析方法论 | [notes/section-7.4-分析方法论.md](./notes/section-7.4-分析方法论.md) |
| 7.5 观测工具 | [notes/section-7.5-观测工具.md](./notes/section-7.5-观测工具.md) |
| 7.6 调优指南 | [notes/section-7.6-调优指南.md](./notes/section-7.6-调优指南.md) |

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
  → 07-Gorman 内核 VM 深入
  → 12-DPDK 大页 / mempool
  → 12-HFT ch05 落地
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

- 上一章：[../chapter-06-cpus/](../chapter-06-cpus/)
- 下一章：[../chapter-08-file-systems/](../chapter-08-file-systems/)
- 应用层分配：[../chapter-05-applications/](../chapter-05-applications/)
- OS 虚拟内存：[../chapter-03-operating-systems/](../chapter-03-operating-systems/)
- USE：[appendix-A-USE方法Linux.md](../appendix-A-USE方法Linux.md)
- 内核 VM 专书：[05-Linux-Virtual-Memory-Manager](../../05-Linux-Virtual-Memory-Manager/)
- CSAPP：[01-CSAPP-3rd Ch9](../../01-CSAPP-3rd/chapter-09-virtual-memory/)
- HFT 调优：[12-HFT ch05](../../15-HFT-Low-Latency-Practice/chapter-05-操作系统内核极致调优/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
