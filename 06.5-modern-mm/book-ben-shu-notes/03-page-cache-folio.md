# 页缓存与 folio API (page → folio)

> 笨叔《奔跑吧 Linux 内核》读书笔记
> 对应旧书: ULK3 / LKD3 (Linux 2.6)
> 对应现代内核: Linux 5.x / 6.x

---

## 本节要点

### 页缓存 (Page Cache) 基本原理

页缓存是内核将磁盘文件数据缓存在物理内存中的机制。读写文件时优先访问页缓存，避免直接 I/O。

```c
// 读写路径 (简化)
// read() → vfs_read() → file_read() → page_cache_sync_readahead()
//                                         → 从磁盘读入 page cache
// write() → vfs_write() → file_write() → 写入 page cache (标记 dirty)
//                                          → writeback 线程异步刷盘
```

### page → folio 迁移

| 操作 | 旧 API (page) | 新 API (folio) | 优势 |
|------|--------------|----------------|------|
| 获取大小 | `PAGE_SIZE << compound_order(page)` | `folio_size(folio)` | 直接返回，不需判断 compound |
| 获取物理地址 | `page_to_phys(page)` (需判断 head) | `folio_phys(folio)` | 自动处理 compound |
| 获取映射 | `page->mapping` (有 compound 歧义) | `folio_mapping(folio)` | 类型安全 |
| dirty 标记 | `SetPageDirty(page)` | `folio_mark_dirty(folio)` | 明确操作级别 |
| 锁定 | `lock_page(page)` | `folio_lock(folio)` | 锁的是整个 folio |

### folio 在页缓存中的使用

```c
// 旧: address_space->page_tree (radix tree of pages)
struct address_space {
    struct radix_tree_root page_tree;  // XArray (5.x+)
    // ...
};

// 新: address_space->i_pages (XArray of folios)
struct address_space {
    struct xarray i_pages;  // 存储 folio 指针 (6.x)
    // ...
};

// 查找页缓存
struct folio *folio = filemap_get_folio(mapping, index);
// 旧: struct page *page = find_get_page(mapping, index);
```

### 大页 folio (Large Folio)

6.x 支持页缓存使用大页 folio（如 2MB），减少 TLB miss 和 page fault 次数：

```bash
# 启用大页 folio (需要内核支持)
echo always > /sys/kernel/mm/transparent_hugepage/enabled
# 或
echo advise > /sys/kernel/mm/transparent_hugepage/enabled  # madvise 控制

# 文件系统层面 (XFS/ext4 支持)
mount -o huge=always /dev/sda1 /mnt/data
```

### readahead 策略

```bash
# 查看当前 readahead 策略
cat /sys/block/sda/queue/read_ahead_kb  # 默认 128KB

# 调整 (HFT 数据文件可增大)
echo 4096 > /sys/block/sda/queue/read_ahead_kb  # 4MB readahead
```

---

## 与旧书对比

| ULK3 / LKD3 (2.6) | 笨叔 (5.x/6.x) | 变化原因 |
|--------------------|-----------------|----------|
| `struct page` 操作页缓存 | `struct folio` 操作页缓存 | 消除 compound page 歧义 |
| radix_tree 管理页缓存 | XArray (5.x+) → folio (6.x) | XArray API 更简洁，RCU-safe |
| 不支持大页页缓存 | 支持 large folio 页缓存 (6.x) | 减少 TLB miss，提高大文件 I/O 效率 |
| `find_get_page()` | `filemap_get_folio()` | 类型安全 |
| `read_cache_page()` | `read_cache_folio()` | 同上 |

---

## 关键数据结构 / 函数

```c
// 源码路径: include/linux/fs.h
struct address_space {
    struct xarray i_pages;           // folio 存储 (XArray)
    struct inode *host;              // 关联的 inode
    struct xa_flags flags;
    // ...
};

// 源码路径: include/linux/page_cache.h
struct folio *filemap_get_folio(struct address_space *mapping, pgoff_t index);
struct folio *filemap_grab_folio(struct address_space *mapping, pgoff_t index);
int filemap_add_folio(struct address_space *mapping, struct folio *folio,
                      pgoff_t index, gfp_t gfp);
```

---

## HFT 关联

- **页缓存预热**：HFT 启动时将行情数据文件 `mmap` + 全部 read，填满页缓存，运行时零磁盘 I/O
- **大页 folio**：6.x 的大页页缓存减少 TLB miss，对大文件随机读性能提升明显
- **避免 dirty writeback**：HFT 不写文件（或写日志用 O_DIRECT 绕过页缓存），避免 writeback 线程抢占 CPU
- **O_DIRECT**：HFT 行情回放用 `O_DIRECT` 绕过页缓存，减少 copy_to_user 一次内存拷贝

---

## 自测

<details>
<summary>Q1: 为什么 folio API 比 page API 更安全？举一个具体 bug 例子。</summary>

旧 API 中 `page->mapping` 对 compound page 的尾页返回 NULL（只有头页有 mapping），开发者容易忘记检查 `PageCompound(page)`，导致 NULL 指针解引用。folio API 中 `folio_mapping(folio)` 对任何 folio 都返回正确的 mapping，编译器类型检查也能防止 folio 和 page 混用。
</details>

<details>
<summary>Q2: 大页 folio (large folio) 对 HFT 有什么实际好处？</summary>

(1) 减少 TLB miss：一个 2MB folio 只需 1 个 TLB 条目，而 512 个 4KB 页需要 512 个；(2) 减少 page fault：2MB folio 一次 fault 代替 512 次；(3) 减少 page cache 查找：XArray 中一个 folio 条目代替 512 个 page 条目。但注意：大页 folio 可能导致内存碎片（需要连续 2MB 物理页），HFT 系统应预留大页。
</details>

<details>
<summary>Q3: HFT 为什么用 O_DIRECT 绕过页缓存？有什么代价？</summary>

原因：(1) 减少一次 copy：O_DIRECT 绕过页缓存，DMA 直接到用户空间；(2) 避免页缓存占用内存，挤压交易数据缓存；(3) 避免 writeback 线程抢占 CPU。代价：(1) I/O 必须 512B/4KB 对齐；(2) 失去页缓存的预读优化；(3) 同一文件被多进程访问时无缓存共享。HFT 行情回放用 O_DIRECT + 预加载到自定义内存池，不依赖页缓存。
</details>
