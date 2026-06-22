## 9.9.7–9.9.14 分配器实现

### 9.9.7–9.9.9 放置、分割、扩展堆

- **首次适配 / 下一次适配 / 最佳适配** — 时间 vs 碎片权衡
- **分割** — 空闲块大于需求时切开
- **堆不够** — `sbrk` 向内核要更多页

### 9.9.10–9.9.11 合并与边界标记

- **释放** 时 **合并相邻空闲块** — 减外部碎片
- **边界标记 (footer)** — O(1) 判断前块是否空闲

### 9.9.12 简单分配器综合

- 课程 lab — 理解 `mm_malloc` / `mm_free` / `mm_realloc`

### 9.9.13 显式空闲链表

- 空闲块内 **指针** 串成链表 — 只遍历空闲块，更快

### 9.9.14 分离空闲链表

- 按 **大小类** 分桶 — 类似 glibc/jemalloc 思想；**O(1)** 常见尺寸

**HFT 对照：**

| 策略 | 场景 |
|------|------|
| **固定大小池** | Order、Event、Fix 消息 |
| **bump allocator** | 每 tick reset 的 arena |
| **Rust** | 栈 + arena crate；避免全局 allocator 锁 |

→ DPDK mbuf 池 · [Ch 5 少 malloc](../chapter-05-optimizing-performance/)

---

← [本章导读](../README.md)
