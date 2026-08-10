# Folio 提案

> **原文:** [Folios for filesystems](https://lwn.net/Articles/849438/) (LWN, 2021)
> **作者:** Matthew Wilcox
> **内核版本:** 5.16+ (初始合入)
> **对标旧书:** ULK3 Ch15 / LKD3 Ch19 (page cache, page API)

---

## 核心观点

Matthew Wilcox 提出 `folio` 类型来解决 `page` 结构体的根本歧义问题。

### page 的问题

Linux 的 `page` 结构体有双重身份：
1. **base page**：一个 4KB 物理页
2. **compound page**：多个连续页组成的大页（head page + tail pages）

大量 API 假设 `page` 是 base page，但实际可能收到 compound page 的 tail page，导致 bug：

```c
// 危险代码: 可能收到 tail page
struct page *page = ...;
void *addr = page_address(page);  // tail page 返回错误地址!
size_t size = PAGE_SIZE;          // 应该是 compound_size(page)!
struct address_space *mapping = page->mapping;  // tail page 的 mapping 是 NULL!
```

### folio 解决方案

```c
// folio 明确表示一个完整的内存对象
struct folio {
    struct page page;  // 包装 head page
};

// folio API 保证操作的是整个 folio
void *folio_address(struct folio *folio);     // 正确地址
size_t folio_size(struct folio *folio);        // 正确大小 (4KB 或 2MB)
struct address_space *folio_mapping(struct folio *folio);  // 正确 mapping
```

### 迁移路径

```
page API → folio API:
  find_get_page()        → filemap_get_folio()
  page_address(page)     → folio_address(folio)
  PAGE_SIZE              → folio_size(folio)
  lock_page(page)        → folio_lock(folio)
  SetPageDirty(page)     → folio_mark_dirty(folio)
  get_page(page)         → folio_get(folio)
  put_page(page)         → folio_put(folio)
```

---

## 与旧书差异

| ULK3 / LKD3 讲的 | 现代实现 |
|-------------------|---------|
| `struct page` 操作页缓存 | `struct folio` 操作页缓存 |
| `page->mapping` 直接访问 | `folio_mapping(folio)` 函数访问 |
| 无 compound page 安全检查 | folio API 编译时类型检查 |

---

## HFT 关联

folio API 本身对 HFT 无直接影响（HFT 不做文件系统开发），但 folio 支持的 large folio（大页页缓存）对 HFT 回放行情数据有帮助。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么不直接修改 page 结构体，而要引入新的 folio 类型？

> page 结构体遍布内核各处（数十万处引用），直接修改风险极高。folio 作为 page 的包装类型，可以渐进式迁移：先在页缓存中引入 folio API，再逐步扩展到其他子系统。编译器类型检查可以在编译期发现 folio/page 混用错误。

**Q2:** folio 和 compound page 的关系是什么？

> folio 包装的是 compound page 的 head page（或 base page）。一个 order-0 folio 对应一个 4KB base page。一个 order-9 folio 对应一个 2MB compound page (512 个 page)。folio API 隐藏了 compound page 的复杂性，开发者不需要关心 head/tail。

</details>
