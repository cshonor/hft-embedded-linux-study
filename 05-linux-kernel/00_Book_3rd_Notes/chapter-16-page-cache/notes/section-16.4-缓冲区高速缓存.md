## ④ 缓冲区高速缓存 · Buffer Cache

| 历史 | 磁盘块经 **buffer_head** 映射到页（Ch 14） |
|------|---------------------------------------------|
| **2.4+** | 独立 **buffer cache** 与 **page cache** **统一** |
| 效果 | 块 **直接缓存在页缓存** — 无双重拷贝、无重复占用 |

```
今天：read 文件块 ──► 页缓存中的一页 ──► 需要时 bio 写盘（Ch 14）
```



<details>
<summary>自测题（点击展开）</summary>

**Q1.** buffer cache 和 page cache 的关系？现代内核还有 buffer cache 吗？

<details><summary>答案</summary>

2.4 之前 buffer cache 缓存磁盘块（512B），page cache 缓存文件页（4KB），两者重复缓存同一数据。2.4 后统一：buffer cache 合并到 page cache 中，buffer_head 只作为页内块偏移的描述符。`free` 命令中的 "buff/cache" 就是 page cache（含 buffer）。现代内核已无独立 buffer cache。

</details>

</details>
---
