# Ch 7 §4 2.6 内核的新变化 → v6.6 演进

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**
> 源码核验：Linux **v6.6**（`mm/vmalloc.c` / `mm/util.c`）

---

## 本节讲什么

本节把「2.4 → 2.6」原书讲的变化，**续写到「→ v6.6」**，形成一条完整演进链。

原书的分水岭是「**分配物理页的时机**」：2.4 边 walk 页表边逐个要页，2.6 改成先攒齐页数组再统一映射。v6.6 在这个骨架上继续堆了三块：**红黑树索引、批量页分配、大页映射**，并催生了一个「智能回落」的兄弟 API `kvmalloc`。

---

## 1. 原书分水岭：2.4 vs 2.6

| | 2.4 | 2.6 |
|---|-----|-----|
| **物理页分配时机** | walk 页表到 PTE 时**逐个** `alloc_page` | `vmalloc` **先分配齐**全部页，放进 `pages[]` 数组 |
| **映射方式** | 边建表边要页 | 一次 `map_vm_area()`（v6.6 的 `vmap_pages_range`）统一插页表 |
| **失败回滚** | 页表建到一半失败，状态散乱 | 页数组先备好，失败**整体回滚**干净 |

**效果**：映射路径更集中，失败更容易整体回滚（原书动机）。现代 `vmalloc.c` 仍保留「**先 reserve VA + 再 map 物理页**」的两阶段思想——§2 的 `__get_vm_area_node` → `__vmalloc_area_node` 就是它的直系后代。

---

## 2. v6.6 的三块演进

### 2.1 地址索引：链表 → 红黑树

原书用 `vm_struct` **单链表**，查找/释放 O(n)；v6.6 引入 `vmap_area` 红黑树 + 双链表（§1），把 `alloc_vmap_area`/`find_vmap_area`/`remove_vm_area` 全部压到 O(log n)。区域多时（数百模块/驱动映射）收益显著。

### 2.2 页分配：逐个 → 批量

原书 2.6 是「先分配齐」（但仍是循环里逐个 `alloc_page`）；v6.6 用 `alloc_pages_bulk_array`（§2.4.1）**一次拿最多 100 页**，减少 Buddy 加锁/关抢占次数。这是「分配齐」之上的又一次**批量化**。

### 2.3 映射粒度：4K → PMD 大页

原书 `map_vm_area` 只按 4K PTE 映射；v6.6 的 `VM_ALLOW_HUGE_VMAP` + `vmap_allow_huge`（§2.4.2）让 `vmap_pages_range` 在满足条件时**直接映射 PMD 级 2MB 大页**，减少页表层级和 TLB miss。大页失败自动回退 4K（`goto again`）。

---

## 3. 智能回落兄弟：`kvmalloc`（`mm/util.c:585`）

vmalloc 是「**只**虚拟连续、物理不保证连续」；kmalloc 是「物理连续但受限于 Buddy 碎片」。两者之间需要一个「**先试物理连续，不行再退虚拟连续**」的入口——这就是 `kvmalloc`：

```c
/* mm/util.c:585 */
void *kvmalloc_node(size_t size, gfp_t flags, int node)
{
    gfp_t kmalloc_flags = flags;
    void *ret;

    if (size > PAGE_SIZE) {                    /* :597 大块才考虑回落 */
        kmalloc_flags |= __GFP_NOWARN;         /* :598 失败不刷屏（有后备） */
        if (!(kmalloc_flags & __GFP_RETRY_MAYFAIL))
            kmalloc_flags |= __GFP_NORETRY;    /* :601 不惊动 OOM killer */
        kmalloc_flags &= ~__GFP_NOFAIL;        /* :604 nofail 语义交给 vmalloc */
    }

    ret = kmalloc_node(size, kmalloc_flags, node);   /* :607 ① 先试物理连续 */

    if (ret || size <= PAGE_SIZE)
        return ret;                            /* :613 成功或太小，直接返回 */

    if (!gfpflags_allow_blocking(flags))       /* :617 不可阻塞则无法回落 */
        return NULL;

    return __vmalloc_node_range(size, 1, VMALLOC_START, VMALLOC_END,
            flags, PAGE_KERNEL, VM_ALLOW_HUGE_VMAP,
            node, __builtin_return_address(0));  /* :632 ② 回落 vmalloc（带大页） */
}
```

| 要点 | 说明 |
|------|------|
| **先物理后虚拟** | 优先 `kmalloc`（物理连续 + 直接映射 + TLB 友好），失败才退 `vmalloc` |
| **克制重试** | 大块时 `__GFP_NORETRY`——因为反正有 vmalloc 后备，**不值得为了物理连续去深度回收/惊动 OOM** |
| **大页兜底** | 回落时传 `VM_ALLOW_HUGE_VMAP`，让 vmalloc 也尽量用大页弥补 TLB 劣势 |
| **统一释放** | 配 `kvfree()`（`mm/util.c:642`）——它能识别是 `kmalloc` 还是 `vmalloc` 分配的，一条释放路径搞定 |

**直觉**：`kvmalloc` 是现代内核里「**大小不定的动态分配**」的事实标准（如 BPF map、网络 buffer 等），它把「物理连续 vs 虚拟连续」的选择**内化成策略**，调用者不用自己判断碎片状况。

---

## 4. vmalloc vs Buddy vs slab 一图

```
需要 N 字节内核内存
        │
        ├─ 小对象、高频、定长？ ──► slab / kmalloc (Ch 8)
        │
        ├─ 物理必须连续、直接映射？ ──► Buddy __get_free_pages (Ch 6)
        │
        ├─ 大小不定、能接受回落？ ──► kvmalloc (mm/util.c)
        │       先 kmalloc（物理连续），失败自动退 vmalloc
        │
        └─ 虚拟连续即可、较大块？ ──► vmalloc (Ch 7)
                VA 连续，物理散页 + 页表拼接
                首次 touch 可能 fault 同步 PTE
                huge vmalloc 用 PMD 大页缓解 TLB miss
```

---

## 5. HFT / 阅读建议

| 场景 | 建议 |
|------|------|
| **低延迟用户态堆** | 物理连续 + 大页 + `mlock`——**不走 vmalloc**（直接映射才没有首访 fault 和 TLB 抖动） |
| **内核驱动大块缓冲** | 能连续则 Buddy；碎片严重用 `kvmalloc` 自动回落；明确要 DMA 可达则用 DMA API |
| **理解 fault 延迟** | vmalloc 区**首访** = 页表懒同步 + 可能 TLB miss 链——冷启动抖动的隐蔽来源 |
| **读源码** | [`mm/vmalloc.c`](https://elixir.bootlin.com/linux/v6.6/source/mm/vmalloc.c) + [`include/linux/vmalloc.h`](https://elixir.bootlin.com/linux/v6.6/source/include/linux/vmalloc.h) + [`mm/util.c`](https://elixir.bootlin.com/linux/v6.6/source/mm/util.c)（`kvmalloc`） |

---

## 6. 衔接

- 上节 [§3 释放非连续区域](./section-3-释放非连续区域.md)：懒释放三棵树
- slab：[Ch8 §4 尺寸缓存与 kmalloc/kfree](../../chapter-08-slab-allocator/notes/section-4-尺寸缓存-与-kmalloc-kfree.md)（`kvmalloc` 的物理连续前身）
- 页分配：[Ch6 §2 页面分配](../../chapter-06-physical-page-allocation/notes/section-2-页面分配.md)

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：原书 2.4 → 2.6 的核心变化是什么？它在 v6.6 里还成立吗？**
A：核心变化是「**分配物理页的时机**」——2.4 边 walk 页表边逐个要页，2.6 先攒齐 `pages[]` 数组再统一 `map_vm_area`。这个「两阶段（先 reserve VA + 再 map 物理页）」骨架在 v6.6 完全保留，只是映射函数改名为 `vmap_pages_range`，并在其上叠加了红黑树/批量/大页三块优化。

**Q2：v6.6 相对 2.6 的三块演进分别解决什么问题？**
A：① **红黑树**（链表 → O(log n)）解决区域多时查找慢；② **批量页分配**（逐个 → `alloc_pages_bulk_array` 100 页/次）解决 Buddy 锁/关抢占开销；③ **大页映射**（4K → PMD）解决页表层级深 + TLB miss 多。三者分别对应「索引结构、分配路径、映射粒度」三个层面。

**Q3：`kvmalloc` 和 `vmalloc` 的区别是什么？为什么要发明它？**
A：`vmalloc` 只保证虚拟连续、物理不保证；`kmalloc` 保证物理连续但受碎片限制。`kvmalloc` 是两者的**策略封装**：先 `kmalloc` 试物理连续（更快、直接映射、TLB 友好），失败且块够大时回落 `vmalloc`。它把「物理 vs 虚拟」的选择内化成自动决策，调用者无需自己判断碎片状况。

**Q4：`kvmalloc` 为什么在 `size > PAGE_SIZE` 时给 kmalloc 加 `__GFP_NORETRY`？**
A：因为**有 vmalloc 后备**，不值得为了物理连续去触发深度回收/内存规整/惊动 OOM killer（注释 `:590-595`）。`__GFP_NORETRY` 让 kmalloc「尽力而为，拿不到就算了」，快速落到 vmalloc 路径，避免在内存压力下卡很久。

**Q5：`kvmalloc` 分配的内存用什么释放？为什么？**
A：用 `kvfree()`（`mm/util.c:642`）。它内部能**识别指针来自 `kmalloc` 还是 `vmalloc`**（通过判断地址是否落在 vmalloc 区），一条路径统一释放。这是 `kvmalloc` 的配套——因为调用者拿到指针时已经不知道它到底走了哪条路径。

</details>
