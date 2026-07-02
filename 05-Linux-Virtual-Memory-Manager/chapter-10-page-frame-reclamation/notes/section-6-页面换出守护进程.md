# Ch 10 §6 页面换出守护进程 (`kswapd`)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

### 6. 页面换出守护进程 (`kswapd`)

| 属性 | 说明 |
|------|------|
| **创建时机** | 系统启动 |
| **平时** | **睡眠** |
| **唤醒** | 某 zone 空闲页 **< `pages_low`**（Ch 2 水位） |
| **工作** | 回收直到 free **回到 ~`pages_high`** |
| **同步路径** | 极端压力下，**普通分配路径**（`GFP_KERNEL`）**同步执行部分 reclaim** — **direct reclaim** |

```
free pages
    high ─── 正常
     low ─── wake kswapd（异步）
     min ─── alloc 路径同步 reclaim（调用方阻塞）
```

**HFT：** 监控 **`allocstall`、`pgscan_*`、`kswapd_*`**（`/proc/vmstat`）；latency 尖刺常与 **direct reclaim + 写盘** 同现。

→ [Ch 6 GFP_KERNEL](../../chapter-06-physical-page-allocation/notes/section-4-GFP-标志与进程标志.md#4-gfp-标志与进程标志-gfp--process-flags) · [Ch 13 OOM](../../chapter-13-out-of-memory-management/)

---
