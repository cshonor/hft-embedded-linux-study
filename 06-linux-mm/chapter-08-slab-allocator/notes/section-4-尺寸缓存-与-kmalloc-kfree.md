# Ch 8 §4 尺寸缓存 (Sizes Cache) 与 `kmalloc` / `kfree`

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**

### 4. 尺寸缓存 (Sizes Cache) 与 `kmalloc` / `kfree`

除 **类型专用 cache**（`kmem_cache_create("my_struct", …)`）外，内核大量 **任意小尺寸** 请求走 **通用尺寸 cache**：

| 特点 | 说明 |
|------|------|
| **2 的幂次档位** | 原书：**32 B ~ 128 KiB** 一系列 **`size-N`** cache |
| **两套** | 常规 + **`size-N(DMA)`** — DMA 可达物理区 |
| **API** | **`kmalloc(size, gfp)`** / **`kfree()`** — 按 size 选最近档位 cache |

**内部碎片仍存在：** `kmalloc(33)` 可能占 **64 B 档** — 比 Buddy 整页好，但 **档内浪费**。

**HFT：** 避免 **`malloc` 任意 size**；热路径 **固定 struct + 池** — 对应 **专用 kmem_cache** 而非泛型 kmalloc。

---
