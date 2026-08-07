#!/usr/bin/env python3
"""07 LKD 各章 README 添加"⚠️ 过时标记"段
LKD 3rd 基于 2.6.34，现为 6.x，标出各章过时内容。
"""
import os

BASE = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "00_Book_3rd_Notes")

# chapter_dir → 过时标记内容
CHAPTERS = {
    "chapter-04-process-scheduling": """## ⚠️ 过时标记（LKD 3rd 基于 2.6.34，现为 6.x）

| LKD 讲的 | 现代变化 | 替代资料 |
|-----------|---------|----------|
| **CFS 调度器** | 6.6 起 **EEVDF** 取代 CFS | [EEVDF Scheduler](https://lwn.net/Articles/969062/) (2024) |
| `vruntime` + 红黑树 | EEVDF 用虚拟截止时间，红黑树仍在但选择逻辑变 | [What is EEVDF?](https://lwn.net/Articles/927168/) |
| `sched_prio_to_weight` | 权重表仍存在，但 EEVDF 算法不同 | [The earliest eligible virtual deadline first](https://lwn.net/Articles/925371/) |
| `SCHED_DEADLINE` | LKD 3rd 未讲（2011 年加入），现代内核重要 RT 策略 | [Deadline scheduling](https://docs.kernel.org/scheduler/sched-deadline.html) |

> **原则**：LKD 比 ULK3 新 5 年，CFS 概念仍有效，但 6.6+ 已切换到 EEVDF。补 LWN EEVDF 系列即可。""",

    "chapter-07-interrupts": """## ⚠️ 过时标记（LKD 3rd 基于 2.6.34，现为 6.x）

| LKD 讲的 | 现代变化 | 替代资料 |
|-----------|---------|----------|
| `do_IRQ()` 路径 | 仍存在但路径简化，IRQ 堆栈处理变化 | [Interrupt handling in Linux](https://lwn.net/Articles/302043/) |
| 中断线程化 | LKD 3rd 仅提及，现代内核广泛使用 threaded IRQ | [Threaded interrupt handlers](https://lwn.net/Articles/302043/) |
| `request_irq()` | 改为 `request_threaded_irq()`（推荐） | [Kernel doc: IRQ](https://docs.kernel.org/core-api/genericirq.html) |
| IPI 机制 | 改用 `smp_call_function()` 系列 | [Kernel doc: IPI](https://docs.kernel.org/core-api/smp.html) |

> **原则**：中断概念框架不变，但 threaded IRQ 是现代重要实践。LKD 的中断处理概念仍有效。""",

    "chapter-08-bottom-halves": """## ⚠️ 过时标记（LKD 3rd 基于 2.6.34，现为 6.x）

| LKD 讲的 | 现代变化 | 替代资料 |
|-----------|---------|----------|
| **tasklet** | **逐渐弃用**，现代内核推荐 workqueue 或 threaded IRQ | [The future of tasklets](https://lwn.net/Articles/830964/) |
| 软中断 (softirq) | 仍存在，但 NAPI 网络软irq路径有演进 | [NAPI and softirq](https://docs.kernel.org/networking/napi.html) |
| `schedule_work()` | workqueue 接口更新，`system_wq` vs `system_highpri_wq` | [Kernel doc: workqueue](https://docs.kernel.org/core-api/workqueue.html) |
| `ksoftirqd` | 仍存在，但调度策略有调整 | [Kernel doc: softirq](https://docs.kernel.org/core-api/softirq.html) |

> **原则**：软中断/workqueue 概念不变，tasklet 正在被淘汰。新代码用 workqueue 或 threaded IRQ。""",

    "chapter-09-kernel-sync-intro": """## ⚠️ 过时标记（LKD 3rd 基于 2.6.34，现为 6.x）

| LKD 讲的 | 现代变化 | 替代资料 |
|-----------|---------|----------|
| 死锁示例 | 概念不变，但现代内核有更多检测工具 | [Lockdep](https://www.kernel.org/doc/Documentation/locking/lockdep-design.txt) |
| 争用与扩展性 | 概念不变，但 per-CPU 和 RCU 更广泛使用 | [What is RCU?](https://lwn.net/Articles/262464/) |

> **原则**：同步概念框架（临界区/竞态/死锁/争用）完全有效，是入门同步的最佳讲解。""",

    "chapter-10-sync-methods": """## ⚠️ 过时标记（LKD 3rd 基于 2.6.34，现为 6.x）

| LKD 讲的 | 现代变化 | 替代资料 |
|-----------|---------|----------|
| **大内核锁 (BKL) §10.7** | **已删除**（2.6.37 完全移除） | [The BKL lives on](https://lwn.net/Articles/400542/) |
| RCU（LKD 仅简述） | Tree RCU、Sleepable RCU 大幅演进 | [What is RCU?](https://lwn.net/Articles/262464/) |
| `atomic_t` | 仍存在，新增 `refcount_t`（防溢出） | [refcount_t](https://lwn.net/Articles/715037/) |
| 顺序锁 | 概念不变，实现细节有更新 | [Kernel doc: locking](https://docs.kernel.org/locking/) |
| `seqlock_t` | 仍存在，但 `seqcount_latch_t` 新增（时间keeping 用） | [Kernel doc: seqlock](https://docs.kernel.org/locking/seqlock.html) |

> **原则**：§10.7 BKL 已过时可跳过。其余同步原语概念不变，RCU 需补 LWN 系列文章。""",

    "chapter-11-timers": """## ⚠️ 过时标记（LKD 3rd 基于 2.6.34，现为 6.x）

| LKD 讲的 | 现代变化 | 替代资料 |
|-----------|---------|----------|
| **`HZ` 固定节拍** | **NOHZ（tickless）** 模式广泛使用 | [NO_HZ: Tickless kernel](https://lwn.net/Articles/229185/) |
| `jiffies` | 仍存在，但高精度计时用 `hrtimer` | [hrtimer subsystem](https://lwn.net/Articles/167897/) |
| `clocksource` 框架 | LKD 3rd 未详述，现代内核核心 | [Clocksource and clockevents](https://docs.kernel.org/timers/timekeeping.html) |
| `timer_list` | 接口变化：`setup_timer()` → `timer_setup()` | [Kernel doc: timers](https://docs.kernel.org/timers/) |
| 动态定时器 | `hrtimer` 提供高精度替代 | [High-resolution timers](https://lwn.net/Articles/167897/) |

> **原则**：jiffies/timer_list 概念仍有效，但 hrtimer/clocksource/nohz 是现代核心。LKD 是入门好材料，但需补 hrtimer 文档。""",

    "chapter-12-memory-management": """## ⚠️ 过时标记（LKD 3rd 基于 2.6.34，现为 6.x）

| LKD 讲的 | 现代变化 | 替代资料 |
|-----------|---------|----------|
| **SLAB 分配器 §12.7** | **SLUB** 取代 SLAB（2.6.23 起默认） | [SLUB: The unqueued slab allocator](https://lwn.net/Articles/229096/) |
| `struct page` | 大量字段移出，改用 **`struct folio`** | [Folios and the page cache](https://lwn.net/Articles/895104/) |
| `kmalloc`/`vmalloc` | 接口仍在，但底层 SLUB 实现不同 | [Slab allocation improvements](https://lwn.net/Articles/887591/) |
| per-CPU 分配器 | 仍存在，但接口和实现有更新 | [Kernel doc: percpu](https://docs.kernel.org/core-api/percpu.html) |
| 高端内存 (highmem) | 64 位内核无 highmem 概念 | [Why folios?](https://lwn.net/Articles/880965/) |

> **原则**：§12.7 SLAB 仅作概念理解（看 SLUB 替代）。`struct page`→`folio` 是重大重构。其余分配器概念仍有效。""",

    "chapter-15-process-address-space": """## ⚠️ 过时标记（LKD 3rd 基于 2.6.34，现为 6.x）

| LKD 讲的 | 现代变化 | 替代资料 |
|-----------|---------|----------|
| **VMA 红黑树 + 链表** | **maple tree** 取代红黑树（6.1 起） | [The maple tree](https://lwn.net/Articles/845507/) |
| `find_vma()` | 改为 maple tree 查找 | [A maple tree for VMA tracking](https://lwn.net/Articles/895690/) |
| `vm_area_struct` | 仍存在，但查找结构变了 | [Maple tree documentation](https://docs.kernel.org/core-api/maple_tree.html) |
| 页表 (4 级) | x86-64 现代 5 级页表（57 位虚拟地址） | [5-level page tables](https://lwn.net/Articles/717293/) |

> **原则**：VMA 概念不变，但数据结构从红黑树到 maple tree。`task_struct→mm_struct→VMA` 层次仍有效。""",
}


def insert_outdated_marker(content, marker):
    """在第一个纯 --- 行后插入过时标记段"""
    lines = content.split("\n")
    for i, line in enumerate(lines):
        if line.strip() == "---":
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
