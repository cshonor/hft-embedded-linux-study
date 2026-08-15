## 1. 本章定位

> **ULK Ch 9 Process Address Space** · 内核如何为用户进程管理 **虚拟内存**

---

### 一、本章讲什么

Ch 8 讲 **物理页框** 怎么分配；本章讲 **用户进程** 如何获得和使用内存：

| 策略 | 含义 |
|------|------|
| **延迟分配** | 先给 **线性地址区间**（VMA），物理页框 **访问时才分配** |
| **缺页驱动** | 真正访问 → 缺页异常 → `do_page_fault()` 分配/映射 |

核心对象：**`mm_struct`**（地址空间）、**`vm_area_struct`**（内存区）、**缺页处理路径**。

---

### 二、小节导航

| 节 | 主题 |
|----|------|
| [2](./section-2-内存描述符.md) | `mm_struct`、`mm_users`/`mm_count`、内核线程 |
| [3](./section-3-内存区VMA.md) | `vm_area_struct`、红黑树 + 链表、`do_mmap`/`do_munmap` |
| [4](./section-4-缺页异常.md) | `do_page_fault()`、SIGSEGV vs 合法缺页 |
| [5](./section-5-请求调页.md) | 匿名页、零页、读/写路径 |
| [6](./section-6-写时复制与堆.md) | COW/`do_wp_page()`、`brk()` 堆管理 |

---

### 三、在 Linux 链上的位置

```
Ch 2  页表 / 线性地址
Ch 3  fork（COW 预告）
Ch 8  物理页分配（伙伴 / Slab）
Ch 9  进程虚拟地址空间（本章）
Ch 10 brk/mmap 系统调用入口
Ch 17 页回收 / swap
```

HFT：**mmap 大页、预 touch、避免缺页抖动** 都建立在本章机制之上。

### 常见陷阱

1. 把 ULK 讲的 VMA 红黑树当现代版——6.1 起 maple tree 取代红黑树管理 VMA
2. 以为进程地址空间只有代码+数据+堆+栈——还有 mmap 区、vdso、[vvar]、[heap] 等，布局复杂
3. 混淆 `mm_struct` 和 `task_struct`——`task_struct` 是进程描述符，`mm_struct` 是地址空间描述符

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** ULK Ch9 的 VMA 管理在现代内核中有什么变化？

<details><summary>答案</summary>

最大变化：VMA 查找结构从红黑树 + 链表改为 **maple tree**（6.1+）。原因：① 红黑树在大量 VMA（如数据库进程数百个 mmap）时查找慢（O(log n)）。② maple tree 是 B-tree 变体，缓存友好，查找 O(log n) 但常数更小。③ maple tree 原生支持范围查询（`find_vma()` 场景）。API 不变：`find_vma()` / `find_vma_intersection()` 内部改用 maple tree。

</details>

**Q2.** 一个进程的 `mm_struct` 中有哪些关键字段？

<details><summary>答案</summary>

`pgd`（PGD 物理地址）、`mmap`（VMA 链表头）、`mm_rb`/`mm_mt`（VMA 树/maple tree）、`mmap_base`（mmap 区起始地址）、`total_vm`（总页数）、`locked_vm`（mlock 页数）、`def_flags`、`mmap_sem`/`mmap_lock`（读写锁）、`cpu_vm_mask`（CPU TLB 掩码）。ULK 时代用 `mmap_sem`，6.x 改名 `mmap_lock`。

</details>

**Q3.** HFT 如何优化进程地址空间布局？

<details><summary>答案</summary>

① `mlockall(MCL_CURRENT | MCL_FUTURE)` 锁定所有页，防 swap。② 预 `mmap` 所有内存区域（避免运行时 VMA 分配）。③ 使用大页减少 TLB 条目。④ `prctl(PR_SET_THP_DISABLE)` 禁用 THP（避免 `khugepaged` 整理引起的延迟）。⑤ 检查 `/proc/[pid]/maps` 确认无意外映射。⑥ NUMA 绑定：`numactl --membind=0`。

</details>

</details>

---

← [Ch 9 导读](../README.md) · 下一节 [2. 内存描述符](./section-2-内存描述符.md)
