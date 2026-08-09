## ③ address_space 与基数树

页缓存 **通用** — 不只缓存「文件」，还缓存任何 **基于页的对象**（含部分 mmap 路径）。

| 结构 | 角色 |
|------|------|
| **`address_space`** | 管理 **缓存条目 + 页 I/O** — 可视为 VMA 的 **「物理页侧」对应物** |
| 关联 | 常挂在 **inode** 上（`inode->i_mapping`） |

#### 基数树 · Radix Tree

| 用途 | 按 **文件偏移** 快速查 **缓存页是否在内存** |
|------|---------------------------------------------|
| 替代 | 2.6 前 **全局哈希** — 锁争用、开销大 |

```
address_space
    └── radix tree: offset ──► struct page *
```

> 新内核中部分实现演进为 **xarray** — 语义仍为 **索引 → page**。

→ **Ch 15** VMA · **Ch 13** inode



<details>
<summary>自测题（点击展开）</summary>

**Q1.** address_space 和 page cache 的关系？

<details><summary>答案</summary>

address_space 是 page cache 的管理单位：每个 inode（文件）对应一个 address_space，其中包含该文件所有缓存页的基数树（radix tree / xarray）。查找文件 offset 对应的缓存页：address_space → xarray → page。mmap 文件时，VMA 的 vm_ops->fault 回调从 address_space 取页。

</details>

</details>
---
