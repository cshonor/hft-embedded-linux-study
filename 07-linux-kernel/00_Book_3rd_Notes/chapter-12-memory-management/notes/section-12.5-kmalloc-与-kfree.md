## ⑤ kmalloc() 与 kfree()

**按字节分配、物理连续** — 内核里 **最常用** 的通用分配器，语义最接近用户态 **`malloc`**，但受 **`gfp_mask`** 与 **上下文** 严格约束。

#### 基本用法

```c
void *ptr = kmalloc(size, GFP_KERNEL);
if (!ptr)
    return -ENOMEM;
/* 使用 */
kfree(ptr);
```

| 属性 | 说明 |
|------|------|
| **物理连续** | 适合 **DMA**（配合 `dma_map_*`）与 **线性访问** |
| **大小上限** | **`KMALLOC_MAX_SIZE`** — 通常 **几 MB 量级**（架构/配置相关） |
| **对齐** | 至少 **字对齐**；`kmalloc(size, gfp | __GFP_DMA)` 等进一步约束 |

#### gfp_mask 常用标志

| 标志 | 行为 | 使用上下文 |
|------|------|------------|
| **`GFP_KERNEL`** | 常规；**可睡眠** 触发回收、直接 reclaim | **进程上下文**、**不持 spinlock** |
| **`GFP_ATOMIC`** | **绝不睡眠** — 用 emergency reserve | **中断、softirq、持 spinlock** |
| **`GFP_KERNEL_ACCOUNT`** | 计入 **memcg**（cgroup 内存 accounting） | 容器环境 |
| **`GFP_DMA`** | 从 **ZONE_DMA** 分配 | 老式 ISA DMA |
| **`GFP_DMA32`** | 物理 **< 4GB** | 常见 **PCI 32-bit DMA** 设备 |
| **`__GFP_ZERO`** | 返回 **清零** 内存 — 类似 `kzalloc` |
| **`__GFP_NOMEMALLOC`** | 不用 **PF_MEMALLOC**  reserve | 避免递归分配路径 |

#### `kzalloc` / `kcalloc`

| API | 等价 |
|-----|------|
| **`kzalloc(size, gfp)`** | `kmalloc` + **memset 0** |
| **`kcalloc(n, size, gfp)`** | 检查 **overflow** 的 n×size 分配 + 清零 |

#### 失败与 OOM

| 路径 | 行为 |
|------|------|
| **`GFP_KERNEL` 失败** | 可能 **同步 reclaim**、写 swap、仍失败 → **NULL** |
| **`GFP_ATOMIC` 失败** | **快速 NULL** — 无睡眠回收 |
| **全局 OOM** | **`out_of_memory()`** — 可能 **杀进程** 腾页（Ch 16 方向） |

```
进程上下文 + GFP_KERNEL:
  kmalloc ──► Slab 命中？──是──► 返回
                │否
                ▼
           页分配器 ──► 睡眠 reclaim ──► 仍失败 ──► NULL

中断 + GFP_ATOMIC:
  kmalloc ──► 仅 reserve / 本地 cache ──► 失败 ──► NULL（快）
```

#### 与 Slab / 页分配器栈

| 大小 | 典型路径 |
|------|----------|
| **≤ slab cache max** | **Slab 快速路径** |
| **较大** | **伙伴系统 alloc_pages** |
| **> KMALLOC_MAX_SIZE** | 用 **`vmalloc`** 或 **`alloc_pages` 多页** |

**HFT：** 用户态 **对象池 / arena** 镜像 **`GFP_ATOMIC` 预分配** — ISR/NAPI 里 **`kmalloc(GFP_ATOMIC)`** 是 **尾延迟炸弹**。实盘驱动：**probe 阶段** `kmalloc` 建 ring，**runtime 零 alloc**。与用户 **`tcmalloc` 线程缓存** 同构：**热路径只 hit cache**。

→ [01 CSAPP Ch9 malloc/池化](../../../../02-computer-systems/chapter-09-virtual-memory/) · [06 Gorman GFP](../../../../09-linux-mm/chapter-06-physical-page-allocation/notes/section-4-GFP-标志与进程标志.md) · [Ch 5 syscall 路径](../../chapter-05-system-calls/)

---
