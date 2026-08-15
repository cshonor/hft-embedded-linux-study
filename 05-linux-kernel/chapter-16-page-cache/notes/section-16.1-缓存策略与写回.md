## ① 缓存策略与写回 · Write-back

Linux 对 **可缓存的页数据** 采用 **写回（write-back）** — 非 no-write、非 write-through。

| 策略 | 行为 |
|------|------|
| **write-back** | 写先进入 **页高速缓存** → 页标 **脏（dirty）** → 入 **脏页链表** → **定期写回磁盘** → 清脏 |

```
应用 write()
    ▼
页缓存（内存）— 立即返回（通常）
    ▼
（稍后）flusher 写回磁盘
```

| 对比 | |
|------|--|
| **write-through** | 每次写都落盘 — 慢、一致性强 |
| **write-back** | 批量异步写 — **快** · 崩溃可能丢未回写数据 |

**HFT：** tick 路径 **不应依赖** 写回完成；关键持久化用 **`fsync`** / 独立日志盘 / **`O_DIRECT`** 自管缓存。

→ [03 SysPerf Ch8 FS](../../../14-systems-performance/chapter-08-file-systems/) · [Ch7 `vm.dirty_*`](../../../14-systems-performance/chapter-07-memory/notes/section-7.6-调优指南.md)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** write-back 和 write-through 的区别？Linux 为什么选 write-back？

<details><summary>答案</summary>

write-through：写同时更新缓存和磁盘 → 数据安全但慢。write-back：只写缓存，标记脏页，后台异步写回磁盘 → 快但断电可能丢数据。Linux 选 write-back 因为：1) 多次写合并为一次 IO（减少磁盘操作）；2) 延迟写让 IO 调度器合并排序。HFT 交易日志不能丢数据 → 用 O_SYNC 或 fsync() 强制写回。

</details>

</details>
---
