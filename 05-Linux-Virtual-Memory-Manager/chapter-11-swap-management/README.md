# Ch 11 交换管理 · Swap Management

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

---

## 本章概述

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过**（HFT：**热路径应尽量避免 swap** — **`mlock` / 足够 RAM / `swapoff`**；理解本章可知 **swap fault 为何是毫秒~秒级**）

物理内存用尽时，Linux 把 **进程私有页 / 匿名页** **复制到后备存储 (backing storage)** — **交换区 (swap area)** — 腾出 **物理页框**（Ch 10 回收链的末端）。

**Swap 的两面作用（原书）：**

| 作用 | 说明 |
|------|------|
| **扩展有效虚拟内存** | 按需 **swap in** — 进程可用 VA 可大于物理 RAM |
| **腾出 RAM 给更热数据** | 换出 **冷匿名页**，把物理内存留给 **页缓存 / 磁盘缓冲** 等 |

**HFT：** 延迟敏感进程 **不应依赖 swap 扩容量** — swap I/O = **不可接受的 tail latency**。

→ 回收触发 swap：[Ch 10 §5](../../chapter-10-page-frame-reclamation/notes/section-5-换出进程页面.md#5-换出进程页面-swapping-out-process-pages) · swap-in fault：[Ch 4 §4.2](../../chapter-04-process-address-space/)

> **时代说明：** 原书 **`swap_info_struct`、`rw_swap_page()`** 等属 2.4/2.6 语境；现代主线 [`mm/swap*.c`](https://elixir.bootlin.com/linux/latest/source/mm/swap_state.c) · [`mm/page_io.c`](https://elixir.bootlin.com/linux/latest/source/mm/page_io.c) 等 — **PTE 存 swap entry、swap cache、cluster 分配** 思想不变。

---

## 本章在 VM 子系统中的位置

```
Ch 10 shrink 选中匿名页 victim
        ↓
Ch 11 分配 swap slot → 写盘 → PTE 改为 swap entry（not present）
        ↓
进程再访问 → Ch 4 page fault → swap in 读盘 → PTE present
```

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 描述交换区 | [notes/section-1-描述交换区.md](./notes/section-1-描述交换区.md) |
| 2. 映射 PTE 到交换项 | [notes/section-2-映射-PTE-到交换项.md](./notes/section-2-映射-PTE-到交换项.md) |
| 3. 分配交换槽 | [notes/section-3-分配交换槽.md](./notes/section-3-分配交换槽.md) |
| 4. 交换缓存  — **核心** | [notes/section-4-交换缓存-—-核心.md](./notes/section-4-交换缓存-—-核心.md) |
| 5. 交换区读写与块 I/O | [notes/section-5-交换区读写与块-I-O.md](./notes/section-5-交换区读写与块-I-O.md) |
| 6. 激活与停用交换区 | [notes/section-6-激活与停用交换区.md](./notes/section-6-激活与停用交换区.md) |
| 7. 2.6 内核的新变化：`swap_extent` | [notes/section-7-2.6-内核的新变化：swap_extent.md](./notes/section-7-2.6-内核的新变化：swap_extent.md) |

---

## 相关章节

- 上一章：[../chapter-10-page-frame-reclamation/](../chapter-10-page-frame-reclamation/)
- 下一章：[../chapter-12-shared-memory-virtual-filesystem/](../chapter-12-shared-memory-virtual-filesystem/)
- 附录 K：[../../appendix-K-交换管理.md](../../appendix-K-交换管理.md)
- 全书目录：[OUTLINE.md](../../OUTLINE.md)
