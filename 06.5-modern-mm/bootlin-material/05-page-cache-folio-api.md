# Bootlin: 页缓存与 folio API

> **来源:** [Bootlin Kernel Training — File Systems](https://bootlin.com/docs/kernel/)
> **主题:** 页缓存、folio API、readahead
> **对标旧书:** ULK3 Ch15 / LKD3 Ch16

---

## 讲义要点

### 页缓存架构

```
应用 read():
  → vfs_read() → generic_file_read()
  → filemap_get_folio(mapping, index)  — 查页缓存
    → 命中: 从 folio 拷贝到用户空间
    → 未命中: page_cache_sync_readahead() → 从磁盘读
  → 返回数据

应用 write():
  → vfs_write() → generic_file_write()
  → filemap_get_folio() 或 filemap_grab_folio() — 查/建页缓存
  → 从用户空间拷贝到 folio
  → folio_mark_dirty(folio)
  → 异步: writeback 线程将 dirty folio 刷盘
```

### folio 在页缓存中的操作

```c
// 源码路径: mm/filemap.c
struct folio *filemap_get_folio(struct address_space *mapping, pgoff_t index);
// 查找页缓存, 不存在返回 NULL

struct folio *filemap_grab_folio(struct address_space *mapping, pgoff_t index);
// 查找, 不存在则分配新 folio 并加入页缓存

int filemap_add_folio(struct address_space *mapping, struct folio *folio,
                      pgoff_t index, gfp_t gfp);
// 将新分配的 folio 加入页缓存

// readahead
void page_cache_sync_readahead(struct address_space *mapping,
                               struct file_ra_state *ra,
                               struct file *file, pgoff_t index,
                               unsigned long req_count);
```

### 大页 folio (Large Folio) 在页缓存

```c
// 6.x: 文件系统可以使用大页 folio 做页缓存
// 例如 XFS 的 large folio 支持
struct folio *filemap_alloc_folio(gfp_t gfp, unsigned int order);
// order=0: 4KB, order=9: 2MB (如果支持)
```

### writeback

```c
// 源码路径: mm/page-writeback.c
// dirty folio 被标记后, writeback 线程异步刷盘

// writeback 触发:
// 1. 定时: dirty_writeback_interval (默认 5 秒)
// 2. 比例: dirty pages > dirty_background_ratio (默认 10%)
// 3. 同步: fsync() / sync()

// HFT 应避免 writeback: 用 O_DIRECT 或只读文件
```

---

## 动手实验

```bash
# 1. 查看页缓存统计
cat /proc/meminfo | grep -E "Cached|Buffers|SwapCached"

# 2. 查看文件的页缓存
# fincore 工具 (需要安装)
fincore /path/to/file
# 或用 vmtouch
vmtouch /path/to/file

# 3. 预热页缓存
cat /path/to/big_file > /dev/null  # 读一遍, 填充页缓存

# 4. 清空页缓存 (危险!)
echo 1 > /proc/sys/vm/drop_caches  # 释放页缓存
echo 3 > /proc/sys/vm/drop_caches  # 释放页缓存 + inode/dentry

# 5. 查看大页 folio
mount -o huge=always /dev/sda1 /mnt/data  # XFS 大页 folio
```

---

## 与旧书差异

| ULK3 | Bootlin 讲义 |
|------|-------------|
| page API | folio API (6.x) |
| radix_tree | XArray + folio |
| 无 large folio | 支持 large folio 页缓存 |
| `read_cache_page()` | `read_cache_folio()` |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** folio 的 `folio_mark_dirty()` 和旧 API `SetPageDirty()` 有什么区别？

> `SetPageDirty(page)` 在传入 tail page 时只设置 tail page 的标志，head page 的 dirty 标志未设置，导致 writeback 线程可能遗漏。`folio_mark_dirty(folio)` 内部找到 head page 并设置，保证整个 folio 的 dirty 状态正确。

**Q2:** readahead 如何提高文件读取性能？

> readahead 在应用请求一个页时，预读后续多个页到页缓存。原理：文件访问通常顺序，预读命中率 >90%。现代内核的 readahead 算法自适应：检测顺序访问模式后增大预读窗口（从 128KB 到 1MB+）。对 HFT 行情回放（大文件顺序读），readahead 减少了 90%+ 的磁盘 I/O 等待。

</details>
