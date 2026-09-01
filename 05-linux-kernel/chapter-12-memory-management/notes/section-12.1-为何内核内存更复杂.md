## ① 为何内核内存更复杂

内核与用户进程 **共用物理 RAM**，但 **分配规则、失败后果、并发约束** 完全不同 — 本章是 **内核侧** 内存管理入门。

#### 用户空间 vs 内核空间

| 维度 | 用户空间 | 内核空间 |
|------|----------|----------|
| 主要 API | `malloc` / `mmap` / `brk` | **`kmalloc` / `vmalloc` / Slab / 页分配器** |
| 失败时 | 常 **`NULL` / OOM killer 杀别人** | **panic / BUG / 子系统停摆** |
| 睡眠 | `malloc` 可阻塞等页 | **视 `GFP_*` 与上下文** — 中断里 **绝不能** |
| 换出 | 匿名页可 **swap** | **内核页一般不可换出**（Ch 2） |
| 泄漏 | 进程退出 **回收 VMA** | 泄漏直到 **重启** |
| 碎片 | libc + mmap 区管理 | **伙伴系统 + Slab** 多层 |

```
用户 fault ──► 缺页 ──► 可能 swap / 读文件
内核 alloc ──► 直接从 free list / Slab 取 ──► 失败则 NULL 或 OOM
```

#### 内核为何「更复杂」

| 原因 | 说明 |
|------|------|
| **多种上下文** | 进程、softirq、hardirq、**持 spinlock** — 同一 API 不同 **gfp_mask** |
| **物理连续需求** | DMA、大页、某些架构 **IOMMU** — 不能任意 `vmalloc` |
| **每 CPU / NUMA** | 本地内存 **更快**；跨 node **延迟** |
| **对象固定大小** | `task_struct`、`inode` — **Slab 缓存** 比通用 malloc 高效 |
| **嵌入式** | RAM 小 — **GFP 失败** 更常见；**静态池** 更普遍 |

#### `gfp_mask`：上下文决定你**能不能等**

`kmalloc(size, gfp_mask)` 的第二参数不是装饰——它声明"**此刻允许内核做什么来凑内存**"：

| gfp_mask | 能否睡眠/触发回收 | 允许的上下文 | 失败时 |
|----------|------------------|--------------|--------|
| `GFP_KERNEL` | ✓ 可睡眠（直接回收、换出） | **进程上下文** | 重试后仍 NULL |
| **`GFP_ATOMIC`** | ✗ **绝不睡眠**（只从预留水位掏） | **中断 / softirq / 持 spinlock** | 高水位线以下直接 **失败返回 NULL** |
| `GFP_NOWAIT` | ✗ 不睡但可轻微回收 | 不能睡又要尽力 | NULL |
| `GFP_DMA / GFP_DMA32` | 限地址范围 | 老式设备 DMA | — |

> **为什么 GFP_ATOMIC 更容易失败**：它不许触发磁盘回收（睡眠），只能从 `min_watermark` 以下的**紧急预留**里掏——所以中断路径的分配失败要先想到"预留水位不够"，而不是"内存真没了"。`/proc/zoneinfo` 的 `min` 与 `VM event: pgalloc` 排查。（预留大小可由 `watermark_scale_factor` 调节。）

```
        进程上下文 ──► GFP_KERNEL ──► 可回收/可睡眠 → 尽力成功
中断/softirq/持spin ──► GFP_ATOMIC ──► 只碰预留水 → 容易失败
```

#### 内核页为何**不可换出**：自举死锁

| 链条 | 说明 |
|------|------|
| 换出一页需要 | 把它写到 **swap 设备** → 需要 **I/O** |
| 执行 I/O 需要 | **内核代码在物理内存里**运行（驱动、块层） |
| 若内核页自己被换出 | 取回它又要执行**换入代码**——**换入代码在哪个页里？** | 

→ **内核文本/核心数据常驻物理内存**（`_text.._end` 永久 resident）；换出机制是为用户页发明的，内核页直接豁免。代价：内核内存泄漏**无法靠重启进程治愈**，只能重启机器——这也是"内存泄漏在服务器上比在桌面更致命"的内核侧根源。

#### 地址空间布局：内核与用户怎么分家

```
32 位经典（LKD3rd 时代，3:1 split）：
 0x00000000 ┌──────────────┐
            │  用户空间 3GB │  每进程独立
 0xBFFFFFFF ├──────────────┤
            │  内核空间 1GB │  所有进程共享同一份映射
 0xFFFFFFFF └──────────────┘
              ↑ 直接映射区（PAGE_OFFSET 起）：物理内存线性映进来
              ↑ vmalloc 区 / kmap 区（HIGHMEM 时代的伤疤）

64 位（x86-64 现行）：
 0xFFFF8000_00000000 起 = 内核空间（128TB）
   ├ 直接映射区 page_offset_base
   ├ vmalloc/modules/bpf 区……
 用户空间可达 128TB —— HIGHMEM 概念整体作废（12.9 节的历史包袱）
```

> `HIGHMEM`（12.9 节）只在 32 位有意义：1GB 内核窗口装不下几十 GB 物理内存，才需要 `kmap` 临时映射。64 位上直接映射区大到覆盖全部物理内存——**读 12.9 时把它当历史课**。

#### 本章 API 地图（先览）

| 层次 | API 族 | 典型对象 |
|------|--------|----------|
| **页** | `alloc_pages` | 连续物理页、DMA 缓冲 |
| **字节（连续物理）** | **`kmalloc`** | 小～中结构、驱动私有数据 |
| **字节（虚连续）** | **`vmalloc`** | 大块非性能关键 |
| **固定类型** | **Slab / `kmem_cache_*`** | 高频内核对象 |
| **每核** | **per-CPU** | 计数器、软中断统计 |
| **HIGHMEM** | **`kmap*`** | 32 位上访问高端物理页 |

**HFT：** 用户态 **预分配 ring buffer / object pool** 对应内核 **Slab + GFP_ATOMIC 预建池** — 热路径 **零分配**。懂本章可读懂 **驱动 probe 失败**（`kmalloc` OOM）、**NUMA 绑内存**（`mbind` 用户态镜像）。

→ [06 Gorman 物理内存描述](../../../06-linux-mm/chapter-02-describing-physical-memory/) · [01 CSAPP Ch9 VM](../../../02-computer-systems/chapter-09-virtual-memory/) · Ch 2 内核 vs 用户 VA



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 内核内存分配和用户态 malloc 的根本区别是什么？

<details><summary>答案</summary>

1) 内核不能用 malloc，用 kmalloc/vmalloc；2) 内核分配受 gfp_mask 约束（能否阻塞、能否等待 IO）；3) 内核分配失败不能返回 NULL 给用户（要正确处理 ENOMEM）；4) 内核分配在中断上下文中不能睡眠（GFP_ATOMIC）；5) 内核内存需要考虑 DMA 约束（物理连续、32 位地址限制）。

</details>

**Q2.** 内核页为什么不能被换出（swap）？

<details><summary>答案</summary>

自举死锁：换出需要写 swap 设备（I/O），执行 I/O 的内核代码自身必须在物理内存中；若内核页被换出，取回它的换入代码同样在（可能被换出的）内核页里——鸡生蛋死锁。所以内核文本与核心数据常驻物理内存，`_text.._end` 区间永久 resident。推论：内核内存泄漏无法通过杀进程回收（不像用户进程退出即回收 VMA），只能重启机器。

</details>

**Q3.** 中断处理程序里 `kmalloc(size, GFP_KERNEL)` 会怎样？应该用什么？

<details><summary>答案</summary>

`GFP_KERNEL` 允许睡眠（直接回收可能等 I/O），而中断上下文无"当前进程"可挂起——睡眠 = panic/调度 bug。必须用 `GFP_ATOMIC`：只从紧急预留水位分配、绝不睡眠；代价是高负载下更容易返回 NULL，调用者必须检查。若中断里频繁分配失败，正确解法通常不是调大预留，而是**把分配挪出中断**（softirq/workqueue 里用 GFP_KERNEL 分配，中断上半部只入队）。

</details>

</details>
---
