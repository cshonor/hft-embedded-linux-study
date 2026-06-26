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

→ [03 SysPerf Ch8 FS](../../../../03-Systems-Performance-2nd/chapter-08-file-systems/) · [Ch7 `vm.dirty_*`](../../../../03-Systems-Performance-2nd/chapter-07-memory/notes/section-7.6-调优指南.md)

---
