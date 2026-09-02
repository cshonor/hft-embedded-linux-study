# Ch 9 §2 映射与解除映射高端页

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**
> 源码核验：Linux **v6.6**（`include/linux/highmem.h` / `mm/highmem.c`）

---

## 本节讲什么

本节回答两个问题：

1. `kmap` 家族在 v6.6 有**三代 API**——`kmap()`、`kmap_local_page()`、`kmap_atomic()`，各是什么关系、谁已废弃？
2. 在 x86_64 上，这些函数**为什么几乎全是 no-op**？

原书只讲 `kmap()`/`kunmap()` 和 `kmap_atomic()`/`kunmap_atomic()` 两代；v6.6 里前者被 `kmap_local_page()` 取代，后者**已废弃**。这是本节最重要的「演进」信息。

---

## 1. v6.6 的 kmap 三代 API

| API | 状态 | 语义 | 返回值 |
|-----|------|------|--------|
| `kmap(page)` | **遗留**，官方称「比 `kmap_local_page` 显著慢」 | 全局 PKMap 槽，可能睡眠 | `void *` |
| `kmap_local_page(page)` | ✅ **现代首选** | 每 CPU 临时映射，不睡眠、可嵌套 | `void *` |
| `kmap_local_folio(folio, off)` | ✅ 现代（folio 版） | 同上，操作 folio 而非 page | `void *` |
| `kmap_atomic(page)` | ⛔ **已废弃** | 「Do not use in new code」 | `void *` |

官方注释（`include/linux/highmem.h`）白纸黑字：

```c
/* :135-146 */
/**
 * kmap_atomic - Atomically map a page for temporary usage - Deprecated!
 * ...
 * In fact a wrapper around kmap_local_page() which also disables pagefaults
 * ...
 * Do not use in new code. Use kmap_local_page() instead.
 */
static inline void *kmap_atomic(struct page *page);

/* :89-94 — kmap_local_page 相对 kmap 的优势 */
/*
 * While kmap_local_page() is significantly faster than kmap() for the highmem
 * case ...
 */
```

**关键认知更新**：原书「常规用 `kmap`，原子上下文用 `kmap_atomic`」的二分法，在 v6.6 已变成「**统一用 `kmap_local_page`**」。`kmap()` 退居「少数需要**全局可等待**映射」的场景，`kmap_atomic()` 是历史遗留。

---

## 2. `kmap()` / `kunmap()`：全局 PKMap（遗留）

```
struct page *page  (HIGHMEM)
    kmap(page)  → 内核可用的 void *vaddr（占用全局 PKMap 槽）
    … 内核读写该页 …
    kunmap(page) → 释放 PKMap 槽
```

| 特点 | 说明 |
|------|------|
| **可能睡眠** | 全局 PKMap 槽满时需等待——**不可在中断/原子上下文用** |
| **必须配对 kunmap** | 否则映射泄漏，槽位被永久占用 |
| **较慢** | 要查/更新 `page_address` 哈希表，还要处理槽位争抢 |

实现上（`mm/highmem.c`）走 `kmap_high()`（:296）/`kunmap_high()`（:348），靠 `page_address()`（:742）的**哈希表**（`page_address_htable[1<<PA_HASH_ORDER]`）跟踪「哪个页映射到了哪个 VA」。

---

## 3. `kmap_local_page()`：现代每 CPU 映射

```c
/* mm/highmem.c:564 — 关键实现 */
void *__kmap_local_page_prot(struct page *page, pgprot_t prot)
{
    void *kmap;

    /* 非 HIGHMEM 页直接返回其固定直接映射地址（见 §4） */
    if (!IS_ENABLED(CONFIG_DEBUG_KMAP_LOCAL_FORCE_MAP) && !PageHighMem(page))
        return page_address(page);

    kmap = arch_kmap_local_high_get(page);          /* 架构的本地映射快取 */
    if (kmap)
        return kmap;
    return __kmap_local_pfn_prot(page_to_pfn(page), prot);  /* 临时改 PTE */
}
```

| 特点 | 说明 |
|------|------|
| **不睡眠** | 用**每 CPU 的临时页表项**（`fixmap` 里预留），无全局争抢 |
| **可嵌套** | 同一 CPU 上可先后映射多页，严格按栈序 `kunmap_local` |
| **极快** | 无哈希表、无锁，直接改本地 PTE |
| **映射只在当前 CPU 有效** | 换 CPU 必须重新映射（映射是 per-CPU 的） |

---

## 4. x86_64 上：全部退化成 `page_address()`

这是最反直觉、也最重要的一点：**在 x86_64 上，`kmap` 家族几乎全是 no-op**。

看 `__kmap_local_page_prot()` 的第一行判断（`mm/highmem.c:573`）：

```c
if (!IS_ENABLED(CONFIG_DEBUG_KMAP_LOCAL_FORCE_MAP) && !PageHighMem(page))
    return page_address(page);   /* ← 直接返回直接映射地址，什么也没映射 */
```

在 x86_64 上 `PageHighMem(page)` **恒为 false**（没有 HIGHMEM），所以 `kmap_local_page()` = `page_address()` = `PAGE_OFFSET + (pfn << PAGE_SHIFT)`——**这个地址在 `paging_init` 建立直接映射时就已经存在了**，`kmap` 只是把它还给你。

```
x86_64:  kmap_local_page(page)  →  page_address(page)  →  直接映射 VA（早已存在）
32 位 HIGHMEM:  kmap_local_page(page)  →  临时改 PTE，映射进 PKMap/fixmap 窗口
```

**为什么还保留这些函数？** 为了**代码可移植**。内核代码写一次，32 位和 64 位都能跑：64 位上它们是「免费的 no-op」，32 位上它们是「真正的临时映射」。这正是内核「**用统一 API 掩盖架构差异**」的典型手法。

---

## 5. HFT / 嵌入式关联

| 现象 | 本节机制的兑现 |
|------|----------------|
| x86_64 上 `kmap` 开销 | 几乎为零（直接 `page_address`），**不构成延迟瓶颈** |
| 32 位嵌入式上 `kmap` | 有真实 PTE 改写 + TLB flush 开销（`arch_kmap_local_post_unmap` 里 `flush_tlb_one_kernel`） |
| 写内核代码 / 读 `mm/` | 统一用 `kmap_local_page`，别再用废弃的 `kmap_atomic` |
| RT 内核 | `kmap_local_page` 不关抢占、不关中断，比 `kmap_atomic` 更适合实时 |

---

## 6. 衔接

- 下节 [§3 回弹缓冲区](./section-3-回弹缓冲区.md)：HIGHMEM 页给设备做 DMA 时的问题
- 原子分配对照：[Ch6 GFP 标志](../../chapter-06-physical-page-allocation/notes/section-4-GFP-标志与进程标志.md)（硬上下文不能用会睡的路径）

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：`kmap()` 和 `kmap_local_page()` 的本质区别是什么？**
A：`kmap()` 用**全局 PKMap 窗口**，槽位有限、可能睡眠等待、要查哈希表，慢；`kmap_local_page()` 用**每 CPU 的临时页表项**，无全局争抢、不睡眠、可嵌套，快。官方注释原话：「kmap_local_page 在 HIGHMEM 场景下**显著快于** kmap」。新代码一律用后者。

**Q2：为什么 `kmap_atomic()` 被废弃？**
A：它的历史作用是「在原子上下文映射且关抢占」，但实现是「关页故障 + 关抢占」的粗暴组合，有副作用。`kmap_local_page()` 用**每 CPU 独立映射**实现同样的「原子上下文可用」，且**不关抢占、不关中断**（只靠 per-CPU 槽位隔离）。官方结论：`kmap_atomic` 只是 `kmap_local_page` 的「关页故障」包装，新代码直接用 `kmap_local_page`。

**Q3：x86_64 上 `kmap_local_page` 为什么「什么也没映射」就返回了？**
A：因为 `__kmap_local_page_prot()` 开头判断 `!PageHighMem(page)`，而 x86_64 没有 HIGHMEM，所有页 `PageHighMem` 都是 false，于是直接 `return page_address(page)`。这个地址在启动建立直接映射时就有了，`kmap` 只是查出来还给你，**零开销**。

**Q4：`page_address()` 在 32 位 HIGHMEM 上怎么工作？**
A：它查一个**哈希表**（`page_address_htable`，按 `page` 指针哈希），表项记录「这个 HIGHMEM 页当前映射在哪个 VA」。`kmap_high()` 映射成功时 `set_page_address()` 把 `page → vaddr` 记进表；`kunmap_high()` 时清掉。非 HIGHMEM 页则直接算 `PAGE_OFFSET + pfn`，不查表。

**Q5：为什么 `kmap_local_page` 的映射「只在当前 CPU 有效」？**
A：因为它用的是**每 CPU 各自的临时页表项**——CPU0 上映射了 page A，改的是 CPU0 的那份页表；CPU1 上的对应页表项还是空的（或映射着别的东西）。所以映射期间**不能换 CPU**（也不能睡眠到别的 CPU 上再访问），这就是它「不睡眠、per-CPU」语义的根源。

</details>

---
