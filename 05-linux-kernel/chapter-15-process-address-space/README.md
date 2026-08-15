# Ch 15 进程地址空间 · The Process Address Space

> **Linux Kernel Development 3rd** · Robert Love · **选读**

> 本章定位：用户态 **虚拟地址空间** 的内核表示 — **`mm_struct`、VMA、mmap/munmap、页表/TLB**。衔接 **Ch 3 COW fork**、**Ch 12 物理页**、HFT **mmap/大页/mlock/零缺页**。

---

## ⚠️ 过时标记（LKD 3rd 基于 2.6.34，现为 6.x）

| LKD 讲的 | 现代变化 | 替代资料 |
|-----------|---------|----------|
| **VMA 红黑树 + 链表** | **maple tree** 取代红黑树（6.1 起） | [The maple tree](https://lwn.net/Articles/845507/) |
| `find_vma()` | 改为 maple tree 查找 | [A maple tree for VMA tracking](https://lwn.net/Articles/895690/) |
| `vm_area_struct` | 仍存在，但查找结构变了 | [Maple tree documentation](https://docs.kernel.org/core-api/maple_tree.html) |
| 页表 (4 级) | x86-64 现代 5 级页表（57 位虚拟地址） | [5-level page tables](https://lwn.net/Articles/717293/) |

> **原则**：VMA 概念不变，但数据结构从红黑树到 maple tree。`task_struct→mm_struct→VMA` 层次仍有效。

---

## 本节结构

| 节 | 主题 | 带走什么 |
|----|------|----------|
| **① 地址空间** | 平坦模型 · 内存区域 | text/data/bss/栈/mmap |
| **② mm_struct** | 内存描述符 | `mm_users` · `mm_count` |
| **③ VMA** | `vm_area_struct` | 权限 · `vm_ops` |
| **④ 链表+树** | mmap / mm_rb | 遍历 vs 查找 |
| **⑤ 操作 VMA** | `find_vma` 等 | `mmap_cache` |
| **⑥ mmap/munmap** | `do_mmap` / `do_munmap` | 合并相邻区 |
| **⑦ 页表** | PGD/PMD/PTE · TLB | VA → PA |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 地址空间 | [notes/section-15.1-地址空间.md](./notes/section-15.1-地址空间.md) |
| 内存描述符 | [notes/section-15.2-内存描述符.md](./notes/section-15.2-内存描述符.md) |
| 虚拟内存区域 | [notes/section-15.3-虚拟内存区域.md](./notes/section-15.3-虚拟内存区域.md) |
| 内存区域的链表与树 | [notes/section-15.4-内存区域的链表与树.md](./notes/section-15.4-内存区域的链表与树.md) |
| 操作内存区域 | [notes/section-15.5-操作内存区域.md](./notes/section-15.5-操作内存区域.md) |
| 创建与删除地址区间 | [notes/section-15.6-创建与删除地址区间.md](./notes/section-15.6-创建与删除地址区间.md) |
| 页表 | [notes/section-15.7-页表.md](./notes/section-15.7-页表.md) |
| 从访问到缺页（概念） | [notes/section-15.8-从访问到缺页概念.md](./notes/section-15.8-从访问到缺页概念.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 地址空间？ | 进程 **虚拟** 可寻址范围 + 多个 **VMA** |
| 内核怎么表示？ | **`mm_struct`** + **`vm_area_struct`** |
| 线程共享？ | 同 `mm` · `mm_users` / `mm_count` |
| VMA 怎么索引？ | **链表遍历** + **红黑树查找** |
| 用户如何改映射？ | **`mmap` / `munmap`** → `do_mmap` / `do_munmap` |
| VA→PA？ | **页表** + **TLB** |
| 内核线程？ | **`mm == NULL`** · 借用他人页表 |

---

## 本章学习目标 · 自检

- [ ] 列举地址空间中 **text/data/bss/栈/mmap** 等典型区
- [ ] 区分 **`mm_users` 与 `mm_count`**
- [ ] 解释为何 VMA 同时需要 **链表和红黑树**
- [ ] 说出 **`find_vma`** 与 **`mmap_cache`** 作用
- [ ] 画 **mmap 合并相邻 VMA** 的直觉
- [ ] HFT：对照 **大页、mlock、预 touch、零缺页** 策略

---

## 相关章节

- 上一章：[../chapter-14-block-io/](../chapter-14-block-io/)
- 下一章：[../chapter-16-page-cache/](../chapter-16-page-cache/)
- 全书导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
