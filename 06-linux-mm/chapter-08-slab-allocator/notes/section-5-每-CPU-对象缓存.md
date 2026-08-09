# Ch 8 §5 每 CPU 对象缓存 (Per-CPU Object Cache)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**

### 5. 每 CPU 对象缓存 (Per-CPU Object Cache)

| 问题 | 方案 |
|------|------|
| 多核抢 **cache->spinlock** | 每 CPU **`array_cache`** — **本地 object 栈** |
| 分配 / 释放 | **优先本地 batch**，**无锁** |
| 本地空 / 满 | **bulk** 与 **全局 slabs_partial / slabs_free** 交换 |

与 [Ch 6 pageset](../../chapter-06-physical-page-allocation/notes/section-6-2.6-内核的新变化.md#6-26-内核的新变化)（**页** 的 per-CPU）并列 — **Slab 做 object 级 per-CPU**。

**HFT 镜像：**

```
Core 0:  local Order[64]  ── pop/push 无锁
         空了 → 从 global pool 一次拿 32 个
Core 1:  同上
```

→ go-dex 若多 goroutine 抢全局 `sync.Pool` — 同类 **锁争用** 问题；**按 P 绑定 pool**（或 Rust thread-local）是 slab 思路。

---
