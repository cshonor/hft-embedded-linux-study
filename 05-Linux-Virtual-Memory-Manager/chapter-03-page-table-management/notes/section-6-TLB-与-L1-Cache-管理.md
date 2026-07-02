# Ch 3 §6 TLB 与 L1 Cache 管理

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**

### 6. TLB 与 L1 Cache 管理

CPU 用 **TLB** 缓存 **最近用过的 VA→PA**；**L1 cache** 缓存 **物理帧内容**。页表更新后，硬件 **不一定** 自动使旧 TLB/cache 项失效。

Linux 在 **改映射、切换 mm、换页** 等路径插入 **架构相关 hook**：

| Hook（示例） | 何时需要 |
|--------------|----------|
| **`flush_tlb_all()`** | 全局 TLB 失效 |
| **`flush_tlb_mm()`** | 某 **`mm_struct`** 的 TLB 项 |
| **`flush_tlb_page()`** | 单页 |
| **`flush_cache_mm()` / `flush_cache_page()`** | 某些 arch 上 **cache alias** / **coherency** |

**上下文切换** 换 **`mm`** → 常 **切换页表基址 + 部分 flush** — **多线程绑核** 可减少 **跨核 TLB shootdown**（仍与 **页表共享** 策略有关）。

**HFT：**

| 现象 | 页表/TLB 视角 |
|------|----------------|
| **工作集 > TLB 覆盖** | **TLB miss** ↑ → 页表 walk ↑ → 延迟尾 |
| **4KiB 小页** | 同样 1GiB 映射需 **更多 PTE / 更多 TLB 项** |
| **2MiB / 1GiB 大页、THP** | **更少 PTE、更少 TLB miss** — HFT 常 **显式 hugepage** 或 **关 THP 防 latency 抖动**（见 THP 笔记） |
| **进程迁移到其他 CPU** | 可能 **remote TLB invalidation** |

→ [Hennessy Ch2 TLB](../02-Computer-Architecture-6th/chapter-02-memory-hierarchy-design/) · [12-DPDK EAL 大页](../12-DPDK-Low-Latency-Network/01-Intro-Book/notes/chapter-01-DPDK架构与EAL/)

---
