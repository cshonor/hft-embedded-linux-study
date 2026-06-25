# 6. 内核内存分析

### `kmem`

追踪 **`kmalloc` 等 Slab 分配** — 按栈统计 **次数、均大小、总字节**。

```bash
sudo kmem-bpfcc 10
```

**场景：** 内核 **泄漏**、某驱动疯狂 alloc。

### `kpages`

追踪 **页级分配** `alloc_pages` — 谁触发 **整页** 分配。

```bash
sudo kpages-bpfcc 10
```

### `slabratetop`

按 **cache 名称** 显示 Slab **分配速率**（实时 top）。

```bash
sudo slabratetop-bpfcc 5
```

**对比 `slabtop`：** 看 **增速** 而非静态快照 — 泄漏/突发 alloc 更敏感。

### `numamove`

**NUMA 页面迁移** 统计 — 自动 NUMA balancing 带来 **意外延迟**。

```bash
sudo numamove-bpfcc
```

**HFT：** 绑核/内存 **local NUMA** 策略下，非零迁移值得查 — 与 [SysPerf Ch 7 内存](../../../02-Systems-Performance-2nd/chapter-07-memory/) 一致。

---
