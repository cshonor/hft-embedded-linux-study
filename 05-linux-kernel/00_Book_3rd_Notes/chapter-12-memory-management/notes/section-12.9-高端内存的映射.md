## ⑨ 高端内存的映射 · High Memory

在 **HIGHMEM zone** 分配的页 **没有** 永久 **内核线性映射地址** — CPU 要访问页内数据，必须先 **临时映射** 到 **fixmap / kmap 槽**。

#### 问题从哪来（x86 32 位典型）

| 事实 | 数值/后果 |
|------|-----------|
| 内核 **直接映射窗口** | 约 **896MB** 物理 @ `PAGE_OFFSET` |
| 超过部分 | **ZONE_HIGHMEM** — **PA 存在，无固定 `page_address()`** |
| 64 位 | 通常 **全 direct map** — 本节作 **概念 + ARM32** 参考 |

```
32-bit 内核 VA 布局（简化）

  0xC0000000 ───────── direct map ─────────► 最多 ~896MB 物理
  0xFFFFFFFF ───────── fixmap / pkmap 窗口（临时映射槽）
```

#### 映射 API

| 方式 | API | 上下文 | 特点 |
|------|-----|--------|------|
| **永久映射（有限槽）** | **`kmap()` / `kunmap()`** | **进程上下文，可睡眠** | **PKMAP 区** 槽位有限 — 耗尽则睡 |
| **原子临时映射** | **`kmap_atomic()` / `kunmap_atomic()`** | **中断、原子上下文** | **关抢占** + **固定 per-CPU 槽** — **必须配对** |
| **HIGHMEM + `__get_free_pages`** | 需 **`__GFP_HIGHMEM`** | 访问前 kmap | 否则 **无法** 读写在页 |

```c
/* 原子上下文读 HIGHMEM 页 */
void *vaddr;
vaddr = kmap_atomic(page);
/* 读写 *vaddr */
kunmap_atomic(vaddr);
```

#### 与 LOWMEM 对比

| | LOWMEM（direct map） | HIGHMEM |
|--|----------------------|---------|
| **`page_address(page)`** | 直接可用 | **NULL** — 须 kmap |
| **中断里访问** | 安全（VA 固定） | **`kmap_atomic` 仅** |
| **性能** | 快 | **映射/解映射开销** |

#### 嵌入式 ARM32

| 场景 | 做法 |
|------|------|
| **小 RAM SoC** | 可能 **全 LOWMEM** |
| **>896MB 物理 @ 32-bit kernel** | 驱动 **DMA / 帧缓冲** 在 HIGHMEM — **必须 kmap_atomic 短访问** |
| **推荐** | 新设计用 **64-bit kernel** 避 HIGHMEM |

**HFT：** 现代 **x86-64 / arm64 交易服务器** 几乎 **碰不到 HIGHMEM** — 但 **`kmap_atomic` 思想** 同构于 **「短临界区访问临时缓冲」**。用户态等价：**mmap 大池 + 指针** 即可；内核 HIGHMEM 是 **VA 不够** 时的 **历史包袱**。

→ [06 Gorman Ch9 高端内存](../../../../06-linux-mm/chapter-09-high-memory-management/) · [Ch 12.3 Zones](./section-12.3-区.md)

---
