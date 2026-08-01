## ④ 获得页 · Getting Pages

当需要 **整页、物理连续**（DMA、页表、大块缓冲）时，走 **页分配器** — 在 **`kmalloc` 上限之上** 或 **Slab 之下** 的原始接口。

#### 核心 API

| API | 返回值 | 说明 |
|-----|--------|------|
| **`alloc_pages(gfp, order)`** | `struct page *` | **2^order** 个连续页 |
| **`__get_free_pages(gfp, order)`** | `unsigned long`（VA） | 页框 + **内核线性地址** |
| **`get_zeroed_page(gfp)`** | VA | **order 0** 且 **清零** — 防泄漏旧数据到用户态 |
| **`free_pages(addr, order)`** | — | 与 `__get_free_pages` 配对 |
| **`__free_pages(page, order)`** | — | 与 `alloc_pages` 配对 |

```c
/* 分配 8 页（order=3 @ 4KB/page = 32KB）连续物理 */
struct page *pg = alloc_pages(GFP_KERNEL, 3);
void *vaddr = page_address(pg);   /* 需 direct map 或已 kmap */

__free_pages(pg, 3);
```

#### `order` 与大小

| order | 页数 @ 4KB | 总大小 |
|-------|------------|--------|
| 0 | 1 | 4 KB |
| 3 | 8 | 32 KB |
| 8 | 256 | 1 MB |
| 10 | 1024 | 4 MB |

#### `gfp_mask` 在此层的含义

| 标志 | 页分配行为 |
|------|------------|
| **`GFP_KERNEL`** | 可 **睡眠** 等内存、触发回收 |
| **`GFP_ATOMIC`** | **不睡眠** — 仅从 reserve / 紧急池 取，**易失败** |
| **`__GFP_ZERO`** | 分配后清零 |
| **`__GFP_HIGHMEM`** | 允许 HIGHMEM 页（须 kmap 访问） |
| **`__GFP_NORETRY`** | 失败 **不重试** 更激进路径 |

#### 与 `kmalloc` 的关系

| 事实 | 说明 |
|------|------|
| **`kmalloc` 内部** | 小对象常从 **Slab**；较大可能 **直接 `alloc_pages`** |
| **`KMALLOC_MAX_SIZE`** | 架构相关上限 — 再大须 **`vmalloc` 或页 API** |

#### 释放纪律

| 错误 | 后果 |
|------|------|
| **漏 `free_pages`** | **永久泄漏** — 该页框永不回 buddy |
| **double free** | 破坏 buddy 链表 — **内存腐败** |
| **order 不匹配** | **BUG** — 必须与分配时 order 相同 |

**HFT：** 网卡驱动 **RX ring** 的 **descriptor + 包缓冲** 常用 **页分配或 dma_pool** — 启动时 **一次性** `alloc_pages`，运行期 **GFP_ATOMIC 零分配**。用户态 **hugepage 池** 同理：**启动预占**，盘中 **不再向内核要连续 2MB**。

→ [06 Gorman Ch6 物理页分配](../../../../09-linux-mm/chapter-06-physical-page-allocation/) · [Ch 12.5 kmalloc](./section-12.5-kmalloc-与-kfree.md)

---
