## 3. 基数树 (Radix Tree) 与标签

> 大文件缓存 — **按页索引 O(1) 量级** 查找

---

### 一、为何不用链表扫描

大文件可能有 **成千上万** 缓存页 — 顺序扫描 **不可接受**。

Linux 2.6 为每个 **`address_space`** 维护一棵 **基数树 (Radix Tree)**：

- **键**：页在文件内的 **索引**（偏移 / 页号）  
- **值**：对应 **`struct page *`**

→ `struct page`：[Ch 8](../chapter-08-memory-management/notes/section-2-页框管理.md)

> **Modern 对照：** 新内核多用 **XArray** 替代 Radix Tree；ULK 概念仍为「索引 → 页」。

---

### 二、基数树标签 (Tags)

在庞大缓存中快速找 **特定状态** 的页 — 树节点为子节点维护 **标签位图**：

| 标签 | 含义 |
|------|------|
| **`PAGECACHE_TAG_DIRTY`** | **脏页** — 内存已改、尚未写回磁盘 |
| **`PAGECACHE_TAG_WRITEBACK`** | **正在写回** 的页 |

查找脏页时可 **跳过整棵无脏子树** — 写回与 sync 路径大幅加速。

→ 写回机制：[section-5](./section-5-回写脏页与pdflush.md)

> **深潜可选：** `radix_tree_tagged()`、`tag_pages_for_writeback` — 见 `lib/radix-tree.c`（historic）。

---

← [2. address_space](./section-2-页缓存与address_space.md) · 下一节 [4. 缓冲页](./section-4-缓冲页与buffer_head.md)
