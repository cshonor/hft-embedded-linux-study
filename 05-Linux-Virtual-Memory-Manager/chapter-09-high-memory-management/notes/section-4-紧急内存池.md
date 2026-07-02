# Ch 9 §4 紧急内存池 (Emergency Pools)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

### 4. 紧急内存池 (Emergency Pools)

**死锁场景：** HIGHMEM I/O 需要 **LOWMEM 做 bounce** → LOWMEM 耗尽 → **I/O 挂起** → 进程阻塞 **无法释放内存** → **无法前进**。

内核为 **bounce** 等保留 **紧急池**：

| 池 | 保留对象 |
|----|----------|
| **页面池** | 至少若干 **可立即用于 bounce 的页** |
| **`buffer_head` 池** | 块 I/O 路径关键结构 |

**保证：** 内存 **再紧** 也能 **完成少量关键 HIGHMEM I/O** — 避免 **全局僵局**。

→ 2.6 泛化为 **[Ch 8 `mempool`](../../chapter-08-slab-allocator/notes/section-6-2.6-内核的新变化.md#内存池-mempool)** — **任何子系统** 可 **预 reserve 关键对象**。

---
