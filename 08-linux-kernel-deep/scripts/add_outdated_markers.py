#!/usr/bin/env python3
"""08 ULK3 各章 README 添加"⚠️ 过时标记"段
在每章 README 的第一个 --- 后、小节笔记前插入过时标记段。
"""
import os

BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# chapter_dir → 过时标记内容
CHAPTERS = {
    "chapter-04-interrupts-and-exceptions": """## ⚠️ 过时标记（ULK3 基于 Linux 2.6，现为 6.x）

| ULK3 讲的 | 现代变化 | 替代资料 |
|-----------|---------|----------|
| IDT 门描述符 (x86-32) | x86-64 IDT 结构不同，中断入口路径重写 | [x86 interrupt handling](https://lwn.net/Articles/107554/) |
| `do_IRQ()` 路径 | 仍存在但路径简化，IRQ 堆栈处理变化 | [Interrupt handling in Linux](https://lwn.net/Articles/302043/) |
| IPI 机制 | 改用 `smp_call_function()` 系列 | [Kernel doc: IPI](https://docs.kernel.org/core-api/smp.html) |
| 中断线程化 | ULK3 时代无，现代内核支持 threaded IRQ | [Threaded interrupt handlers](https://lwn.net/Articles/302043/) |

> **原则**：ULK3 用来理解中断概念框架（IDT/门/上半部/下半部），现代实现查 bootlin 中断训练材料 + 源码 `kernel/irq/`。""",

    "chapter-05-kernel-synchronization": """## ⚠️ 过时标记（ULK3 基于 Linux 2.6，现为 6.x）

| ULK3 讲的 | 现代变化 | 替代资料 |
|-----------|---------|----------|
| **大内核锁 (BKL)** | **已删除**（2.6.37 完全移除） | [The BKL lives on](https://lwn.net/Articles/400542/) |
| RCU 基础版 | Tree RCU、Sleepable RCU、Tasks RCU 大幅演进 | [What is RCU?](https://lwn.net/Articles/262464/) (Paul McKenney) |
| `read_lock()` | 仍存在，但 RCU 更推荐用于读多写少 | [Tree RCU](https://lwn.net/Articles/305782/) |
| `atomic_t` | 仍存在，新增 `refcount_t`（防溢出） | [refcount_t](https://lwn.net/Articles/715037/) |
| 顺序锁 | 概念不变，但实现细节有更新 | [Kernel doc: locking](https://docs.kernel.org/locking/) |

> **原则**：同步原语的概念（自旋锁/信号量/RCU/顺序锁）不变，但 BKL 已删、RCU 大幅演进，务必补 LWN 文章。""",

    "chapter-07-process-scheduling": """## ⚠️ 过时标记（ULK3 基于 Linux 2.6，现为 6.x）

| ULK3 讲的 | 现代变化 | 替代资料 |
|-----------|---------|----------|
| **O(1) 调度器** | 2.6.23 起 **CFS** 取代；6.6 起 **EEVDF** 取代 CFS | [CFS scheduling](https://lwn.net/Articles/230501/) (2007) |
| 优先级数组 + 时间片 | vruntime + 红黑树（CFS）；EEVDF 用虚拟截止时间 | [EEVDF Scheduler](https://lwn.net/Articles/969062/) (2024) |
| `recalc_task_prio()` | **已删除** | [What is EEVDF?](https://lwn.net/Articles/927168/) |
| `runqueue` 结构 | `cfs_rq` → `eevdf_rq`，数据结构重写 | [The earliest eligible virtual deadline first](https://lwn.net/Articles/925371/) |

> **原则**：ULK3 的 O(1) 调度器已完全过时。CFS（2.6.23-6.5）和 EEVDF（6.6+）是两代全新设计。务必读 LWN EEVDF 系列文章。""",

    "chapter-08-memory-management": """## ⚠️ 过时标记（ULK3 基于 Linux 2.6，现为 6.x）

| ULK3 讲的 | 现代变化 | 替代资料 |
|-----------|---------|----------|
| **SLAB 分配器** | **SLUB** 取代 SLAB（2.6.23 起默认） | [SLUB: The unqueued slab allocator](https://lwn.net/Articles/229096/) |
| `kmem_cache` 结构 | SLUB 简化了结构，接口变化 | [Slab allocation improvements](https://lwn.net/Articles/887591/) |
| **`struct page`** | 大量字段移出，改用 **`struct folio`** | [Folios and the page cache](https://lwn.net/Articles/895104/) |
| 页框管理 `__GFP_*` | flag 更新，GFP 接口调整 | [Why folios?](https://lwn.net/Articles/880965/) |

> **原则**：SLAB→SLUB、page→folio 是两大重构。ULK3 的 Slab 分配器章节仅作概念理解，现代实现查 bootlin 内存管理训练材料。""",

    "chapter-09-process-address-space": """## ⚠️ 过时标记（ULK3 基于 Linux 2.6，现为 6.x）

| ULK3 讲的 | 现代变化 | 替代资料 |
|-----------|---------|----------|
| **VMA 红黑树 + 链表** | **maple tree** 取代红黑树（6.1 起） | [The maple tree](https://lwn.net/Articles/845507/) |
| `vm_area_struct` | 仍存在，但查找结构变了 | [A maple tree for VMA tracking](https://lwn.net/Articles/895690/) |
| `find_vma()` | 改为 maple tree 查找 | [Maple tree documentation](https://docs.kernel.org/core-api/maple_tree.html) |
| 缺页处理路径 | 概念不变，但 `fault` 回调接口更新 | [Kernel doc: mm](https://docs.kernel.org/admin-guide/mm/) |

> **原则**：VMA 管理从红黑树到 maple tree 是数据结构层面的重构。`task_struct→mm_struct→VMA` 的层次不变，但查找路径完全不同。""",

    "chapter-10-system-calls": """## ⚠️ 过时标记（ULK3 基于 Linux 2.6，现为 6.x）

| ULK3 讲的 | 现代变化 | 替代资料 |
|-----------|---------|----------|
| `sys_call_table` | x86-64 仍用，但入口改用 `syscall` 指令 | [System call table for x86-64](https://blog.rchapman.org/posts/Linux_System_Call_Table_for_x86_64/) |
| **`0x80` 软中断入口** | **已废弃**，改用 `syscall` 指令 | [vDSO and system calls](https://lwn.net/Articles/627232/) |
| 参数验证 | 概念类似，但 helper 函数更新 | [Kernel doc: syscall API](https://docs.kernel.org/core-api/syscalls.html) |
| `sys_*` 命名 | 现代 `SYSCALL_DEFINE*` 宏 | [Kernel doc: syscall wrappers](https://docs.kernel.org/core-api/syscalls.html) |

> **原则**：系统调用概念框架不变（用户态→内核态切换、参数传递、验证），但入口机制和命名约定已变。""",

    "chapter-14-block-devices": """## ⚠️ 过时标记（ULK3 基于 Linux 2.6，现为 6.x）

| ULK3 讲的 | 现代变化 | 替代资料 |
|-----------|---------|----------|
| **单队列块层** | **multiqueue (blk-mq)** 取代单队列 | [Multiqueue block layer](https://lwn.net/Articles/552904/) |
| `request_queue` 单队列 | 改为 per-CPU 软件队列 + 硬件队列 | [Block I/O latency controller](https://lwn.net/Articles/716107/) |
| I/O 调度器 | deadline/cfq 被替换为 mq-deadline/kyber/none | [Block layer multi-queue design](https://docs.kernel.org/block/blk-mq.html) |

> **原则**：块层从单队列到 blk-mq 是架构级重构。ULK3 的块设备章节几乎全部过时，务必查 blk-mq 文档。""",

    "chapter-15-page-cache": """## ⚠️ 过时标记（ULK3 基于 Linux 2.6，现为 6.x）

| ULK3 讲的 | 现代变化 | 替代资料 |
|-----------|---------|----------|
| **page cache + 基数树** | **folio** + **maple tree** (6.1+) | [Folios and the page cache](https://lwn.net/Articles/895104/) |
| **`pdflush` 线程** | 已被 **`flusher`** 线程取代（per-device） | [Why folios?](https://lwn.net/Articles/880965/) |
| `address_space` | 仍存在，但操作 `folio` 而非 `page` | [Folios for filesystems](https://lwn.net/Articles/931584/) |

> **原则**：page→folio 是页缓存层面的核心重构。ULK3 的页缓存章节仅作概念理解。""",

    "chapter-16-file-access": """## ⚠️ 过时标记（ULK3 基于 Linux 2.6，现为 6.x）

| ULK3 讲的 | 现代变化 | 替代资料 |
|-----------|---------|----------|
| **AIO** (`aio_read`/`aio_write`) | **io_uring** 取代 AIO（5.1+） | [io_uring](https://lwn.net/Articles/776703/) (Jens Axboe) |
| `aio_read()`/`aio_write()` | 仍存在但已不推荐新代码使用 | [io_uring and networking](https://lwn.net/Articles/810414/) |
| `epoll` | 仍存在，io_uring 可替代部分场景 | [Efficient IO with io_uring](https://kernel.dk/io_uring.pdf) |

> **原则**：AIO→io_uring 是异步 I/O 的完全重写。ULK3 的 AIO 章节已过时，io_uring 是现代高性能 I/O 的核心。""",

    "chapter-17-page-reclaim": """## ⚠️ 过时标记（ULK3 基于 Linux 2.6，现为 6.x）

| ULK3 讲的 | 现代变化 | 替代资料 |
|-----------|---------|----------|
| **LRU 双链表** (active/inactive) | **Multi-generational LRU** (MGLRU, 6.1+) | [Multi-generational LRU](https://lwn.net/Articles/856931/) |
| `shrink_zone()` | 重写为 MGLRU 回收路径 | [MGLRU documentation](https://docs.kernel.org/admin-guide/mm/multigen_lru.html) |
| OOM killer | 仍存在但策略可配置 (cgroup OOM) | [Cgroup-aware OOM killer](https://lwn.net/Articles/704179/) |

> **原则**：LRU→MGLRU 是页回收算法的重构。ULK3 的回收路径仅作概念理解，现代实现查 MGLRU 文档。""",
}


def insert_outdated_marker(content, marker):
    """在第一个纯 --- 行后插入过时标记段（不匹配表格分隔线 |---|）"""
    lines = content.split("\n")
    for i, line in enumerate(lines):
        if line.strip() == "---":
            # 在这行后插入标记
            lines.insert(i + 1, "")
            lines.insert(i + 2, marker)
            lines.insert(i + 3, "")
            lines.insert(i + 4, "---")
            return "\n".join(lines)
    return content


ok = 0
skip = 0
miss = 0

for ch_dir, marker in CHAPTERS.items():
    readme_path = os.path.join(BASE, ch_dir, "README.md")
    if not os.path.exists(readme_path):
        print(f"MISSING: {ch_dir}/README.md")
        miss += 1
        continue

    with open(readme_path, "r", encoding="utf-8") as f:
        content = f.read()

    if "⚠️ 过时标记" in content:
        print(f"SKIP: {ch_dir}")
        skip += 1
        continue

    new_content = insert_outdated_marker(content, marker)
    with open(readme_path, "w", encoding="utf-8") as f:
        f.write(new_content)

    print(f"OK: {ch_dir}")
    ok += 1

print(f"\n=== Done: {ok} OK, {miss} MISSING, {skip} SKIP ===")
