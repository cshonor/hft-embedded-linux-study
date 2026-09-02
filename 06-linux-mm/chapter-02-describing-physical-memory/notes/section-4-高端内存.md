# Ch 2 §4 高端内存 (High Memory)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**
> 源码核验：Linux **v6.6**（`enum zone_type` 中的 `CONFIG_HIGHMEM` 条件分支）

---

## 本节讲什么

一个**纯 32 位时代**的问题，但理解它才能看懂大量历史代码。本节回答：

1. 32 位内核为什么「看得见装不下」——highmem 怎么来的？
2. `kmap()/kunmap()` 到底在干什么？
3. 为什么 x86_64 上 `ZONE_HIGHMEM` 干脆消失了？

---

## 1. 32 位内核的地址空间困境

32 位内核只有 **4GiB 虚拟地址空间**，默认按 **3:1 分割**：低 3GiB 给用户态，高 1GiB 给内核。内核的 1GiB 里，通常拿 **~896MiB 做「线性直接映射」**（`PAGE_OFFSET + 物理地址` 就是虚拟地址），剩下 ~128MiB 留给 vmalloc、临时映射等。

```
32 位 x86 内核地址空间（4GiB 总）
┌──────────────────────────────┬──────────────────────────────┐
│  低 3GiB：用户态              │  高 1GiB：内核态              │
│  （进程私有，随 CR3 切换）      │  ┌────────────────────────┐ │
│                              │  │ 直接映射 ~896MiB          │ │
│                              │  │ (PAGE_OFFSET + PA)        │ │ ← ZONE_NORMAL
│                              │  ├────────────────────────┤ │
│                              │  │ vmalloc / 临时映射 ~128MiB │ │ ← kmap 区域
│                              │  └────────────────────────┘ │
└──────────────────────────────┴──────────────────────────────┘
```

**困境：** 如果机器装了 4GiB 甚至更多物理内存，内核那 896MiB 直接映射窗口**盖不住**所有物理页。盖不住的部分就叫 **`ZONE_HIGHMEM`**。

| 问题 | 内核做法 |
|------|----------|
| 物理内存超过直接映射窗口（1GiB–64GiB，PAE 下更大） | 落在 `ZONE_HIGHMEM` |
| 内核不能随时用「`PAGE_OFFSET + PA`」访问它 | 需 `kmap()` **临时映射**进那 128MiB 窗口，用完 `kunmap()` |

**关键：highmem 页不是「不能访问」，而是「不能一直映射着」**——内核要临时把它钉进有限的虚拟地址窗口，用完立刻还。

---

## 2. `kmap()` / `kunmap()`：临时映射

```c
void *kmap(struct page *page);    /* 把 highmem 页临时映射进内核地址空间 */
void kunmap(struct page *page);   /* 解除映射 */

void *kmap_atomic(struct page *page);  /* 原子上下文变体（per-CPU 固定槽） */
void kunmap_atomic(void *addr);
```

- `kmap()`：可睡眠，映射进全局临时映射区，**并发有限**（槽位是全局共享的，用多了要等）
- `kmap_atomic()`：**原子上下文专用**（中断/持锁时不能睡），用 **per-CPU 固定槽**，无竞争但要立刻 `kunmap_atomic()`
- 访问期间若页被换出/迁移，映射会失效——所以 highmem 页 + swap 是老内核里一堆 bug 的来源

**v6.6 现实：** x86_64 上 `kmap()` 基本退化为 `page_address()`（所有物理页都在直接映射里，虚拟地址 = `PAGE_OFFSET + PFN * PAGE_SIZE`）。真正需要临时映射的只剩 32 位 highmem 架构和 `ZONE_DEVICE` 之类特殊内存。

---

## 3. 为什么 x86_64 上 highmem 消失了

回到 §2 的 `enum zone_type`（`mmzone.h:715`）：

```c
#ifdef CONFIG_HIGHMEM
    ZONE_HIGHMEM,
#endif
```

`ZONE_HIGHMEM` 被 `CONFIG_HIGHMEM` 包着，而 **x86_64 不定义 `CONFIG_HIGHMEM`**——因为 64 位内核地址空间大到（48-bit VA = 256TiB，5 级 paging 下 128PiB）可以**把全部物理内存都直接映射**，根本不存在「盖不住」的问题。于是 `ZONE_HIGHMEM` 这个名字在 64 位机器上**不编译进枚举**。

| 架构 | `CONFIG_HIGHMEM` | `ZONE_HIGHMEM` |
|------|------------------|----------------|
| 32 位 x86（i386） | 可选定义 | 有（默认 3:1 分割） |
| x86_64 / ARM64 | 不定义 | **无** |
| 32 位 ARM（部分） | 定义 | 有 |

---

## 4. HFT / 嵌入式关联

| 现象 | 本节机制的兑现 |
|------|----------------|
| 现代 x86_64 服务器基本与 highmem 无关 | `kmap()` 退化为 `page_address()`，无临时映射开销 |
| 嵌入式 32 位 SoC 仍可能踩到 | 32 位 ARM 上 highmem 还在，`kmap_atomic` 的 per-CPU 槽位有限 |
| 「非直接映射」思想仍在 | `vmalloc` 区、`ioremap`、`ZONE_DEVICE` 都是「不能一直直接映射」的现代翻版 |

**HFT 结论：** 纯 x86_64 低延迟场景可以把 highmem 当历史课；但要读老驱动、或做 32 位嵌入式时，`kmap/kunmap` 与 per-CPU 槽位的概念必须清楚。

---

## 5. 衔接

- 上节 [§3 物理页框](./section-3-物理页框.md)：`struct page` 的 `virtual` 字段（`WANT_PAGE_VIRTUAL` 时）就是给 kmap 存临时地址用的
- [§5 2.6 内核的新变化](./section-5-2.6-内核的新变化.md)：本章收尾
- 专章：[Ch 9 高端内存管理](../../chapter-09-high-memory-management/)（原书专章，x86_64 可当背景读）

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：highmem 页是「内核访问不了」吗？**
A：不是「访问不了」，是「不能一直直接映射」。内核得临时 `kmap()` 把它钉进有限的虚拟窗口，用完 `kunmap()`。真正的限制是**同时能映射的 highmem 页数量**受窗口大小约束，而不是单个页不可达。

**Q2：`kmap()` 和 `kmap_atomic()` 什么时候用哪个？**
A：能睡的上下文（进程上下文、未持自旋锁）用 `kmap()`；**原子上下文**（中断 handler、持自旋锁、preempt disabled）必须用 `kmap_atomic()`，因为它用 per-CPU 固定槽、绝不睡眠。混用会导致死锁或「睡在原子上下文」的 BUG。

**Q3：x86_64 上 `kmap()` 为什么能退化成 `page_address()`？**
A：因为 64 位内核地址空间足够大，**所有物理内存都进了直接映射区**，任何页的虚拟地址都能用 `PAGE_OFFSET + PFN * PAGE_SIZE` 算出来，无需临时映射。`page_address(page)` 直接返回这个地址，`kmap()` 变成一行。

**Q4：`struct page` 里那个 `virtual` 字段是干嘛的，为什么 `#if defined(WANT_PAGE_VIRTUAL)`？**
A：在没有全量直接映射的架构（highmem 架构）上，内核需要一个地方存「这个页当前被 kmap 到哪个虚拟地址」。有全量直接映射的架构（x86_64）定义 `WANT_PAGE_VIRTUAL` 为假，省掉这 8 字节——**又一处按架构裁剪 `struct page` 体积**。

**Q5：为什么说 highmem + swap 是老内核 bug 温床？**
A：页被 `kmap()` 映射后，如果它在换出/迁移中被回收，映射就指向了失效内容。老内核要在映射期间**钉住引用计数**（`get_page`）防止回收，漏了就是 use-after-free 或读脏数据。现代内核靠 folio + 严格的 pin 语义把这些坑堵上了。

</details>

---
