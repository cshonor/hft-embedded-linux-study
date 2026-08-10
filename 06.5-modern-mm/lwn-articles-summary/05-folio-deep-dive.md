# Folio 深入

> **原文:** [The folio API](https://lwn.net/Articles/862108/) (LWN, 2021)
> **作者:** Matthew Wilcox
> **对标旧书:** ULK3 Ch15 (page cache 实现细节)

---

## 核心观点

本文深入介绍 folio API 的设计细节和使用模式。

### folio 的层次结构

```
folio (逻辑层)
  ├── head page (物理层: compound head)
  │     ├── tail page 0
  │     ├── tail page 1
  │     └── ... (2^order - 1 个 tail pages)
  └── folio 元数据 (内嵌在 head page 的 struct page 中)
```

### 关键 folio 操作

```c
// 源码路径: include/linux/page_ext.h, mm/folio-compat.c

// 获取 folio
struct folio *page_folio(struct page *page);  // page → folio
struct folio *filemap_get_folio(struct address_space *mapping, pgoff_t index);

// folio 属性
size_t folio_size(struct folio *folio);           // 大小 (4KB/2MB/...)
unsigned int folio_order(struct folio *folio);     // order (0/9/...)
pgoff_t folio_index(struct folio *folio);          // 页缓存索引
struct address_space *folio_mapping(struct folio *folio);

// folio 状态
bool folio_test_uptodate(struct folio *folio);
bool folio_test_dirty(struct folio *folio);
bool folio_test_writeback(struct folio *folio);

// folio 操作
void folio_mark_dirty(struct folio *folio);
void folio_clear_dirty(struct folio *folio);
void folio_lock(struct folio *folio);        // 锁定整个 folio
void folio_unlock(struct folio *folio);
void folio_get(struct folio *folio);          // 引用计数 +1
void folio_put(struct folio *folio);          // 引用计数 -1
```

### 大页 folio (Large Folio) 支持

```c
// 6.x: 文件系统可以使用大页 folio
struct folio *filemap_alloc_folio(gfp_t gfp, unsigned int order);
// order=0: 4KB, order=9: 2MB (需要 CONFIG_TRANSPARENT_HUGEPAGE)

// 检查是否是大页 folio
bool folio_test_large(struct folio *folio);
```

---

## 与旧书差异

| ULK3 讲的 | 现代实现 |
|-----------|---------|
| `page_cache` 用 radix_tree 存 page | 用 XArray 存 folio |
| `add_to_page_cache()` | `filemap_add_folio()` |
| 无大页页缓存 | 支持 large folio 页缓存 |

---

## HFT 关联

large folio 减少 TLB miss。HFT 行情回放用大文件，large folio 页缓存可以减少 512 倍 TLB 条目占用。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** `folio_lock()` 锁定的是什么？和 `lock_page()` 有什么区别？

> `folio_lock()` 锁定整个 folio（可能是 4KB 或 2MB），设置 head page 的 PG_locked 标志。`lock_page()` 在传入 tail page 时行为不正确（可能只锁 tail page 的标志位）。`folio_lock()` 内部正确找到 head page 并锁定。

**Q2:** 大页 folio 在页缓存中的优势是什么？什么条件下启用？

> 优势：(1) 1 个 TLB 条目覆盖 2MB 而非 512 个 4KB 条目；(2) page fault 次数减少 512 倍；(3) XArray 中 1 个条目代替 512 个。启用条件：(1) CONFIG_TRANSPARENT_HUGEPAGE=y；(2) 文件系统支持 (XFS/ext4)；(3) mount -o huge=always 或 madvise。

</details>
