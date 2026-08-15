# Folio 合入状态

> **原文:** [The status of folios](https://lwn.net/Articles/893852/) (LWN, 2022)
> **内核版本:** 5.16 ~ 6.x (渐进式合入)
> **对标旧书:** ULK3 Ch15 (page cache 完全重写)

---

## 核心观点

folio API 从 5.16 开始合入，分多个内核版本渐进式迁移整个页缓存子系统。

### 合入时间线

| 版本 | 合入内容 | 覆盖范围 |
|------|---------|---------|
| 5.16 | folio 基础类型 + 页缓存核心 API | mm/filemap.c |
| 5.17 | folio 在 readahead 路径 | mm/readahead.c |
| 5.18 | folio 在 writeback 路径 | mm/page-writeback.c |
| 5.19 | folio 在 truncate 路径 | mm/truncate.c |
| 6.0 | folio 在 migrate 路径 | mm/migrate.c |
| 6.1 | large folio 支持 | mm/huge_memory.c |
| 6.3+ | 文件系统 folio 迁移 (XFS/ext4) | fs/ |

### 迁移统计

```
5.16: ~2000 行代码改为 folio API
6.0:  ~8000 行代码改为 folio API
6.3+: 几乎所有页缓存路径使用 folio
```

### 仍保留的 page API

部分子系统仍用 page API（暂未迁移）：
- 网络栈 (`sk_buff` 仍用 page)
- DMA 层 (DMA 映射用 page 物理地址)
- 部分 arch 代码 (TLB flush 用 page)

---

## 与旧书差异

| ULK3 讲的 | 现代实现 |
|-----------|---------|
| 全部 page API | 页缓存用 folio，网络/DMA 仍用 page |
| `find_get_page()` | `filemap_get_folio()` (页缓存) |
| `add_to_page_cache_lru()` | `filemap_add_folio()` |

---

## HFT 关联

folio 合入改善了页缓存效率，间接有利于 HFT 的文件 I/O 路径。但 HFT 主要走自定义内存池或 O_DIRECT，页缓存路径影响较小。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么 folio 迁移要分多个内核版本而不是一次性合入？

> 页缓存代码涉及 mm/ 子系统的大半，一次性修改风险极高（编译错误、性能回归、隐蔽 bug）。分版本合入允许每个阶段充分测试和稳定后再进入下一阶段。这也是 Linux 内核开发的惯例——大重构分步进行。

</details>
