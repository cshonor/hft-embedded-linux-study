# Ch 3 §4 内核页表初始化 (Kernel Page Tables)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**

### 4. 内核页表初始化 (Kernel Page Tables)

分页 **非上电即有** — 启动时分阶段建立。

| 阶段 | 做什么 |
|------|--------|
| **引导 (Bootstrapping)** | 仅为 **前 8MiB** 物理内存建页表 — 够 **开启 MMU / 分页单元** |
| **最终化 (Finalizing)** | **`paging_init()`** → **`pagetable_init()`** 等 — 为 **`ZONE_DMA` + `ZONE_NORMAL`** 建立 **内核线性映射**，并初始化 zone 等 |

**用户进程页表** 在 **fork / exec / mmap** 时逐步填充；**内核部分** 在 boot 末段 **全局就绪**。

→ 与 [Ch 5 启动内存分配器](../../chapter-05-boot-memory-allocator/) 衔接（boot 页分配器 vs 正式 page allocator）。

---
