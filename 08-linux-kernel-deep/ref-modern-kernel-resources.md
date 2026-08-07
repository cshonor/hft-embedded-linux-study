# ULK3 过时章节 → 现代 6.x 内核替代资料

> **痛点**: ULK3 (Understanding the Linux Kernel, 3rd, 2005) 基于 Linux 2.6，
> 内核已迭代到 6.x，大量子系统实现已完全重写。
> 本文档为 ULK3 每个过时章节提供现代替代资料（LWN / bootlin / 源码文档）。

---

## 一、纸质书对比

| 书 | 作者 | 版本/年份 | 对应内核 | 评价 |
|---|---|---|---|---|
| **LKD** (Linux Kernel Development) | Robert Love | 3rd (2010) | 2.6.34 | 你已有（07 模块）。比 ULK3 新 5 年，概述性强，仍停在 2.6 |
| **Professional Linux Kernel Architecture** | Wolfgang Mauerer | 2008 | 2.6.24 | 略新于 ULK3，但同样过时，不推荐 |
| **Linux Kernel Programming** | Kaiwan Billimoria | 2021 | 5.x | 较新，覆盖现代驱动和内核模块实践 |
| **Linux Kernel Programming Part 2** | Kaiwan Billimoria | 2021 | 5.x | Char driver、内存、调度进阶 |
| **Linux 内核深度解析** | 余华兵 | 2019 | 4.x | 国内作者，讲调度/内存/RCU 较现代 |
| **奔跑吧 Linux 内核**（4 卷） | 笨叔 | 2022-2024 | 5.x/6.x | **国内跟进最勤**，覆盖到 6.x，分卷讲调度/内存/驱动/文件系统 |

> 纸质书核心痛点：内核迭代太快，出版即过时。上面只有"奔跑吧 Linux 内核"跟到 6.x。

---

## 二、ULK3 过时章节 → LWN 文章映射

LWN.net 是内核开发者写的技术深度文章，覆盖每次重大改动，是纸质书的真正替代品。

### Ch 7 — Process Scheduling（调度器）

| ULK3 讲的 | 现代变化 | LWN 文章 |
|-----------|---------|----------|
| O(1) 调度器 | 2.6.23 起 CFS 取代 O(1)；6.6 起 EEVDF 取代 CFS | [CFS scheduling](https://lwn.net/Articles/230501/) (2007) |
| 优先级数组 + 时间片 | vruntime + 红黑树 | [The earliest eligible virtual deadline first](https://lwn.net/Articles/925371/) (EEVDF, 2023) |
| `recalc_task_prio()` | 已删除 | [EEVDF Scheduler](https://lwn.net/Articles/969062/) (2024) |
| `runqueue` 结构 | `cfs_rq` → `eevdf_rq` | [What is EEVDF?](https://lwn.net/Articles/927168/) |

### Ch 8 — Memory Management（内存管理 / Slab）

| ULK3 讲的 | 现代变化 | LWN 文章 |
|-----------|---------|----------|
| SLAB 分配器 | SLUB 已取代 SLAB（2.6.23 起默认） | [SLUB: The unqueued slab allocator](https://lwn.net/Articles/229096/) |
| `kmem_cache` 结构 | SLUB 简化了结构 | [Slab allocation improvements](https://lwn.net/Articles/887591/) |
| `struct page` | 大量字段移出，改用 `struct folio` | [Folios and the page cache](https://lwn.net/Articles/895104/) |
| 页框管理 | `__GFP_*` flag 更新 | [Why folios?](https://lwn.net/Articles/880965/) |

### Ch 9 — Process Address Space（VMA）

| ULK3 讲的 | 现代变化 | LWN 文章 |
|-----------|---------|----------|
| VMA 红黑树 + 链表 | **maple tree** 取代红黑树（6.1 起） | [The maple tree](https://lwn.net/Articles/845507/) |
| `vm_area_struct` | 仍存在，但查找结构变了 | [A maple tree for VMA tracking](https://lwn.net/Articles/895690/) |
| `find_vma()` | 改为 maple tree 查找 | [Maple tree documentation](https://docs.kernel.org/core-api/maple_tree.html) |

### Ch 10 — System Calls（系统调用）

| ULK3 讲的 | 现代变化 | LWN 文章 |
|-----------|---------|----------|
| `sys_call_table` | x86-64 仍用，但入口改用 `syscall` 指令 | [System call table for x86-64](https://blog.rchapman.org/posts/Linux_System_Call_Table_for_x86_64/) |
| `0x80` 软中断 | 已废弃，改用 `syscall` 指令 | [vDSO and system calls](https://lwn.net/Articles/627232/) |
| 参数验证 | 仍类似，但 helper 函数更新 | [Kernel doc: syscall API](https://docs.kernel.org/core-api/syscalls.html) |

### Ch 4 — Interrupts and Exceptions（中断）

| ULK3 讲的 | 现代变化 | LWN 文章 |
|-----------|---------|----------|
| IDT 门描述符 (x86 32) | x86-64 IDT 结构不同 | [x86 interrupt handling](https://lwn.net/Articles/107554/) |
| `do_IRQ()` | 仍存在但路径简化 | [Interrupt handling in Linux](https://lwn.net/Articles/302043/) |
| IPI 机制 | 改用 `smp_call_function()` | [Kernel doc: IPI](https://docs.kernel.org/core-api/smp.html) |

### Ch 5 — Kernel Synchronization（同步）

| ULK3 讲的 | 现代变化 | LWN 文章 |
|-----------|---------|----------|
| 大内核锁 (BKL) | **已删除** (2.6.37 完全移除) | [The BKL lives on](https://lwn.net/Articles/400542/) |
| RCU 基础版 | Tree RCU、Sleepable RCU、Tasks RCU | [What is RCU?](https://lwn.net/Articles/262464/) (Paul McKenney) |
| `read_lock()` | 仍存在，但 RCU 更推荐用于读多写少 | [Tree RCU](https://lwn.net/Articles/305782/) |
| 原子操作 | `atomic_t` 仍在，新增 `refcount_t` | [Refcount_t](https://lwn.net/Articles/715037/) |

### Ch 14 — Block Device Drivers（块设备层）

| ULK3 讲的 | 现代变化 | LWN 文章 |
|-----------|---------|----------|
| 单队列块层 | **multiqueue** (blk-mq) 取代单队列 | [Multiqueue block layer](https://lwn.net/Articles/552904/) |
| `request_queue` 单队列 | 改为 per-CPU 软件队列 + 硬件队列 | [Block I/O latency controller](https://lwn.net/Articles/716107/) |
| I/O 调度器 | deadline/cfq 被替换为 mq-deadline/kyber/none | [Block layer multi-queue design](https://docs.kernel.org/block/blk-mq.html) |

### Ch 15 — Page Cache（页缓存）

| ULK3 讲的 | 现代变化 | LWN 文章 |
|-----------|---------|----------|
| page cache + 基数树 | **folio** + **mapple tree** (6.1+) | [Folios and the page cache](https://lwn.net/Articles/895104/) |
| `pdflush` 线程 | 已被 `flusher` 线程取代（per-device） | [Why folios?](https://lwn.net/Articles/880965/) |
| `address_space` | 仍存在，但操作 `folio` 而非 `page` | [Folios for filesystems](https://lwn.net/Articles/931584/) |

### Ch 16 — File Access（异步 I/O）

| ULK3 讲的 | 现代变化 | LWN 文章 |
|-----------|---------|----------|
| AIO | **io_uring** 取代 AIO（5.1+） | [io_uring](https://lwn.net/Articles/776703/) (Jens Axboe) |
| `aio_read()`/`aio_write()` | 仍存在但已不推荐 | [io_uring and networking](https://lwn.net/Articles/810414/) |
| `epoll` | 仍存在，io_uring 可替代部分场景 | [Efficient IO with io_uring](https://kernel.dk/io_uring.pdf) |

### Ch 17 — Page Frame Reclaiming（页回收）

| ULK3 讲的 | 现代变化 | LWN 文章 |
|-----------|---------|----------|
| LRU 双链表 (active/inactive) | **Multi-generational LRU** (MGLRU, 6.1+) | [Multi-generational LRU](https://lwn.net/Articles/856931/) |
| `shrink_zone()` | 重写为 MGLRU 回收路径 | [MGLRU documentation](https://docs.kernel.org/admin-guide/mm/multigen_lru.html) |
| OOM killer | 仍存在但策略可配置 (cgroup OOM) | [Cgroup-aware OOM killer](https://lwn.net/Articles/704179/) |

---

## 三、官方文档（永远最新）

| 资源 | URL / 路径 | 优势 |
|------|-----------|------|
| 内核 Documentation/ | `Documentation/` 子目录 | 跟着源码走，永远最新 |
| kernel.org 文档 | https://docs.kernel.org/ | 在线版，按主题分册 |
| 源码注释 | 源码里的 `/* */` | 最权威 |
| 内核内存管理 | https://docs.kernel.org/admin-guide/mm/ | MM 子系统文档 |
| 调度器 | https://docs.kernel.org/scheduler/ | CFS / EEVDF 文档 |
| 锁机制 | https://docs.kernel.org/locking/ | 自旋锁 / RCU / 信号量 |

---

## 四、bootlin 训练材料（强烈推荐）

**免费、持续更新到最新 LTS、讲义 + 源码配套，质量比大多数付费书高。**

| 主题 | URL |
|------|-----|
| Linux kernel 入门 | https://bootlin.com/docs/kernel/ |
| 内核调度与实时 | https://bootlin.com/docs/realtime/ |
| ARM64 架构 | https://bootlin.com/docs/arm/ |
| 设备树 | https://bootlin.com/docs/device-tree/ |
| 内存管理 | https://bootlin.com/docs/memory-management/ |
| 文件系统 | https://bootlin.com/docs/filesystems/ |

---

## 五、KernelTeaching（免费实验课程）

- **GitHub**: https://linux-kernel-labs.github.io/
- **维护**: EPFL (瑞士联邦理工)
- **特点**: 按主题分 lab，配套 QEMU 虚拟机实验
- **覆盖**: 系统调用、中断、调度、内存、同步、文件系统

---

## 六、社区邮件列表

| 列表 | 订阅地址 | 适合 |
|------|---------|------|
| LKML | https://lkml.org/ | 所有补丁的原始讨论 |
| linux-mm | https://lore.kernel.org/linux-mm/ | 内存子系统 |
| linux-sched | https://lore.kernel.org/linux-sched/ | 调度器 |

> 适合跟特定子系统演进，不适合建立框架。

---

## 七、推荐学习路线（HFT + 树莓派 5 场景）

```
建立框架：  LKD（07 模块，你已有）+ 笨叔《奔跑吧 Linux 内核》（补 5.x/6.x）
深度补缺：  LWN 文章（针对 ULK3 过时章节逐个补，见上方映射表）
真实实现：  树莓派 5 源码 + 官方 Documentation/
配套实验：  bootlin 训练材料 + KernelTeaching
```

### 特别推荐

1. **《奔跑吧 Linux 内核》笨叔** — 国内唯一跟到 6.x 的内核书，分 4 卷，针对 ARM64 专门讲。对树莓派 5 (Cortex-A76) 场景对口
2. **bootlin 训练材料** — 免费且持续更新到最新 LTS，讲义 + 源码配套，质量比大多数付费书高

---

## 八、ULK3 仍有价值的章节（未过时或概念仍有效）

| 章节 | 价值 | 说明 |
|------|------|------|
| Ch 1 Introduction | 概念 | 基本概念不变 |
| Ch 2 Memory Addressing | 概念 | 分段/分页概念仍有效，但 Linux 四级页表需查现代文档 |
| Ch 3 Processes | 概念 | `task_struct` 概念不变，字段变化大 |
| Ch 6 Timing | 部分 | jiffies 概念在，但 `hrtimer` / `clocksource` 框架是现代的 |
| Ch 11 Signals | 概念 | 信号机制概念不变，实现细节有变 |
| Ch 19 IPC | 概念 | System V IPC 概念不变 |
| Ch 20 Program Execution | 概念 | ELF 加载概念不变 |

> 原则：ULK3 用来**理解概念框架**，LWN/bootlin 用来**看现代实现**。
