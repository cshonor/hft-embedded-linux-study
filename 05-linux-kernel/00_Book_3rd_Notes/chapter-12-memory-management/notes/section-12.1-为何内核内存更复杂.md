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

→ [06 Gorman 物理内存描述](../../../../06-linux-mm/chapter-02-describing-physical-memory/) · [01 CSAPP Ch9 VM](../../../../02-computer-systems/chapter-09-virtual-memory/) · [Ch 2 内核 vs 用户 VA](../../chapter-02/getting-started-with-the-kernel/)

---
