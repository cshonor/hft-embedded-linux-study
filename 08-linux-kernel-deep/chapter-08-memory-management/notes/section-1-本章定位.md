## 1. 本章定位

> **ULK Ch 8 Memory Management** · 内核如何分配 **物理 RAM**

---

### 一、本章讲什么

除 CPU 外，**内存**是内核最宝贵的资源。本章从 **物理页框** 出发，讲三层分配机制：

| 层次 | 机制 | 解决什么 |
|------|------|----------|
| 页框 | **伙伴系统** | 连续物理页、外碎片 |
| 小对象 | **Slab / kmalloc** | 内核数据结构、内碎片 |
| 大块非连续物理 | **vmalloc** | 物理散、虚拟连续 |

Ch 2 讲地址翻译；Ch 9 讲 **进程虚拟地址空间**；本章讲 **内核如何管理物理页本身**。

---

### 二、小节导航

| 节 | 主题 |
|----|------|
| [2](./section-2-页框管理.md) | `struct page`、Zone、伙伴系统、per-CPU 缓存 |
| [3](./section-3-Slab分配器.md) | Slab、着色、kmalloc、mempool |
| [4](./section-4-非连续内存与vmalloc.md) | vmalloc/vfree、高端内存 kmap |

---

### 三、在 Linux 链上的位置

```
Ch 2  寻址 / 页表
Ch 8  物理页分配（本章）
Ch 9  进程 VMA、缺页、COW
Ch 17 页回收
07 Gorman  VM 专著
```

HFT：**大页 / NUMA 本地分配 / 预分配池** 都建立在本章机制之上。

### 常见陷阱

1. 把 ULK 讲的 buddy system / slab 当现代版——SLUB 取代 SLAB、`struct folio` 取代 `struct page` 作为基本管理单元
2. 以为 `kmalloc()` 分配的是物理连续内存——是的，但虚拟地址在直接映射区，所以物理连续 = 虚拟连续
3. 混淆 `GFP_KERNEL` 和 `GFP_ATOMIC`——前者可睡眠（可能触发回收），后者不睡眠（紧急分配）

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** ULK Ch8 讲的内存管理在现代内核中有哪些重大变化？

<details><summary>答案</summary>

① SLAB → SLUB（2.6.23 默认）：去掉 per-CPU 的 slab 队列，简化结构，减少内存开销。② `struct page` → `struct folio`（6.1+）：folio 是一组连续的 page，减少遍历开销。③ buddy system 仍存在但 API 更新（`alloc_pages()` → `folio_alloc()`）。④ NUMA 感知增强（per-node 页池）。⑤ MGLRU 取代传统 LRU 回收。

</details>

**Q2.** `GFP_KERNEL` / `GFP_ATOMIC` / `GFP_DMA` 的区别和使用场景？

<details><summary>答案</summary>

`GFP_KERNEL`：进程上下文分配，可睡眠（允许磁盘 I/O 回收内存），最常用。`GFP_ATOMIC`：不睡眠（中断/softirq/持锁时），从紧急预留池分配，可能失败。`GFP_DMA`：要求 <16MB 物理地址（老 DMA 设备）。组合：`GFP_KERNEL | __GFP_NOWARN`。HFT 内核模块在热路径应用 `GFP_ATOMIC`（但不能失败），更好的做法是预分配 `mempool`。

</details>

**Q3.** HFT 用户态如何选择内存分配策略？

<details><summary>答案</summary>

① 小对象：`malloc`/`new`（glibc ptmalloc2 或 jemalloc）。② 大块连续：`mmap(MAP_ANONYMOUS | MAP_HUGETLB)`（2MB 大页）。③ 物理连续（DMA 场景）：`/dev/hugepages` + `mmap`。④ 内存池：预分配 + 自管理，避免运行时分配。⑤ 零拷贝：`mmap` 映射文件/设备。关键：`mlockall(MCL_CURRENT | MCL_FUTURE)` 防止换出。

</details>

</details>

---

← [Ch 8 导读](../README.md) · 下一节 [2. 页框管理](./section-2-页框管理.md)
