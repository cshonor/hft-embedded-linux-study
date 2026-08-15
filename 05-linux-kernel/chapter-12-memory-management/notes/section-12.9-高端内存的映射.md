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

→ [06 Gorman Ch9 高端内存](../../../06-linux-mm/chapter-09-high-memory-management/) · [Ch 12.3 Zones](./section-12.3-区.md)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** kmap 和 kmap_atomic 的区别？哪个更高效？

<details><summary>答案</summary>

kmap：可睡眠、用全局锁保护 fixmap 槽、可能阻塞。kmap_atomic：不可睡眠、per-CPU fixmap 槽、无需锁、极快。高频场景用 kmap_atomic（如网络包处理中映射 HIGHMEM 页到内核地址）。x86_64 没有 HIGHMEM，这两个函数是空操作（直接返回 page_address）。

</details>

**Q2.** 为什么 x86_64 不需要高端内存？

<details><summary>答案</summary>

x86_64 有 48 位虚拟地址空间（256TB），而物理内存通常 < 256TB。内核直接映射区（PAGE_OFFSET 开始）可以覆盖所有物理内存，不需要临时映射。高端内存是 32 位系统的限制：32 位内核只有 896MB 直接映射区，超过部分需要 HIGHMEM 机制。

</details>

</details>
---
