## 3. 反向映射 (Reverse Mapping)

> 回收共享页 — 必须找到并 **清空所有 PTE**

---

### 一、问题

一页物理页框可能被 **多个进程 / 多个 VMA** 映射 — 回收前需 **unmap  everywhere**，否则 dangling mapping。

正向：VMA + 偏移 → PTE → 物理页（缺页路径熟悉）。

**反向：** 给定 **物理页** → 哪些 PTE 指向它？

---

### 二、匿名页：`anon_vma`

对 **匿名页**（堆、栈、匿名 mmap）：

- **`anon_vma`** 将映射 **同一页框** 的所有 **`vm_area_struct`** 链成 **双向循环链表**  
- 回收时遍历链表 → 逐个 **清 PTE**  

→ 匿名页：[Ch 9 section-5](../chapter-09-process-address-space/notes/section-5-请求调页.md)

---

### 三、文件映射页：优先搜索树 (PST)

对 **文件映射页**（页缓存-backed）：

- 每个 **`address_space`** / 相关结构维护 **PST**  
- 内存区索引：**起始、结束、大小** — **O(log n)** 找到映射了 **指定文件页索引** 的所有 VMA  

→ `address_space`：[Ch 15 section-2](../chapter-15-page-cache/notes/section-2-页缓存与address_space.md)

> **深潜可选：** PST 节点排序键（radix + heap index）— ULK 2.6 `mm/rmap.c`。

> **Modern 对照：** 新内核 rmap 实现演进（如 maple tree 等）；ULK 抓 **对象级反向映射** 概念。

---

← [2. PFRA](./section-2-PFRA与页分类.md) · 下一节 [4. LRU](./section-4-LRU链表.md)
