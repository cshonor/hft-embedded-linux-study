# Ch 5 §3 内存分配与释放 (Alloc / Free)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**
> 源码核验：Linux **v6.6**（`mm/memblock.c` 的 `memblock_alloc_try_nid` / `memblock_phys_alloc` / `memblock_free`）

---

## 本节讲什么

本节回答两个问题：

1. 启动早期要分配内存（页表、`struct page` 数组、临时结构），**API 是什么、返回的是物理地址还是虚拟地址**？
2. `memblock_free` 在早期**为什么几乎是「假释放」**，真正的释放发生在什么时候？

原书 API（`alloc_bootmem*` / `free_bootmem`）已随 bootmem 一起删除，v6.6 全换成 `memblock_*`。

---

## 1. 分配 API：物理地址 vs 虚拟地址

memblock 的分配函数分**两大类**，关键区别是**返回值类型**：

| 函数（`memblock.h`） | 返回 | 说明 |
|---------------------|------|------|
| `memblock_phys_alloc(size, align)` | `phys_addr_t` **物理地址** | 底层，直接给物理区间 |
| `memblock_alloc(size, align)` | `void *` **虚拟地址** | 包装：物理分配 + `__va()` 转虚拟，**且清零** |
| `memblock_alloc_low(size, align)` | `void *` | 限定在低端内存（DMA 可达） |
| `memblock_alloc_node(size, align, nid)` | `void *` | 指定 NUMA 节点分配 |
| `memblock_alloc_try_nid(size, align, min, max, nid)` | `void *` | 万能入口：指定地址范围 + 节点 + 方向 |

```c
/* memblock.h:423 — memblock_alloc 就是"物理分配 + 转虚拟 + 清零" */
static __always_inline void *memblock_alloc(phys_addr_t size, phys_addr_t align)
{
    return memblock_alloc_try_nid(size, align, MEMBLOCK_LOW_LIMIT,
                                  MEMBLOCK_ALLOC_ACCESSIBLE, NUMA_NO_NODE);
}
```

**`memblock_alloc_try_nid()` 的真身**（`memblock.c:1631`）：

```c
void * __init memblock_alloc_try_nid(phys_addr_t size, phys_addr_t align,
                                     phys_addr_t min_addr, phys_addr_t max_addr,
                                     int nid)
{
    void *ptr = memblock_alloc_internal(size, align,
                                        min_addr, max_addr, nid, false);
    if (ptr)
        memset(ptr, 0, size);   /* ← 注意：分配的内存被清零 */
    return ptr;
}
```

| 要点 | 说明 |
|------|------|
| **返回值 `void *` 是虚拟地址** | 内部用 `__va()` 把物理地址转成直接映射区的虚拟地址 |
| **自动清零** | `memset(ptr, 0, size)`——启动期数据「一拿到就是 0」，避免脏数据 |
| **`align` 对齐** | 页表要求 4K 对齐、大页要求 2M/1G，`align` 参数满足 |
| **`min_addr`/`max_addr`** | 限定地址窗（如 `memblock_alloc_low` 就是 `max_addr = 低端上限`） |
| **`nid`** | NUMA 上指定节点；`NUMA_NO_NODE` = 任意 |

**典型用途**：`paging_init()` 里分配**页表**、`free_area_init()` 里分配 **`struct page`(memmap) 数组**、`setup_arch` 里分配**临时 pgd**——这些全是「分配一次、用一生」的元数据。

---

## 2. `memblock_free`：早期的「假释放」

```c
/* memblock.h:130 */
void memblock_free(void *ptr, size_t size);
```

**关键约束**（也是原书 `free_bootmem` 的核心约束的延续）：

| 规则 | 后果 |
|------|------|
| 早期 free 只是**把区间从 `reserved` 表移除/标记** | 但**没有 `struct page`**，无法进 Buddy，页仍「游离」 |
| 直到 `memblock_free_all()`（§4）才真正进 Buddy | 在此之前 free 是**记账层面**的 |
| 页内部分占用不跟踪 | 与 bootmem 一样，按区间/页粒度，不管理页内碎片 |

**为什么可接受**：启动期分配的内存**几乎伴随系统一生**（页表、memmap、伙伴系统元数据），**很少真正 free**。真正需要「还给系统」的，等 `memblock_free_all()` 一次性移交 Buddy 即可。

---

## 3. 分配方向：top-down vs bottom-up

`memblock.bottom_up`（`memblock.h:92`）控制分配往哪头找空闲区间：

| 方向 | `bottom_up` | 场景 |
|------|-------------|------|
| **自顶向下** | `false`（x86 常见） | 从高地址往低找，**避开低端 DMA/BIOS shadow**，且利于固件把内核放高位 |
| **自底向上** | `true` | 从低地址往高找，某些嵌入式/特殊固件要求 |

```
内存地址低 ────────────────────────────────► 高
  bottom_up=true:  从低往高找第一个够大的空闲区间
  bottom_up=false: 从高往低找
```

**HFT 关联**：这个方向选择决定了**内核镜像、页表、memmap 落在物理内存的哪一段**，进而影响后续 NUMA 节点的内存分布。虽然用户态感知不到，但它是「物理内存布局」的第一块拼图（后续 Ch6 的 zone 划分、Ch2 的 node 边界都建立在它之上）。

---

## 4. HFT / 嵌入式关联

| 现象 | 本节机制的兑现 |
|------|----------------|
| 页表/`struct page` 占了多少物理内存 | 启动期 `memblock_alloc` 分配，`dmesg` 里 `Memory: ...` 行可见 |
| 物理内存布局决定 NUMA 亲和 | `memblock_alloc_try_nid(..., nid)` 让元数据**就近**落在所属节点 |
| 嵌入式裁剪启动内存占用 | 理解「哪些是必须 reserve 的元数据」才能做减法 |

---

## 5. 衔接

- 下节 [§4 启动内存分配器的退役](./section-4-启动内存分配器的退役.md)：free 的「假」在哪一步变「真」
- 物理页分配：[Ch6 物理页分配](../../chapter-06-physical-page-allocation/)（Buddy 接管后的正主）

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：`memblock_phys_alloc` 和 `memblock_alloc` 都「分配内存」，为什么返回类型不同？**
A：`memblock_phys_alloc` 返回 **`phys_addr_t`（物理地址）**，是底层原语；`memblock_alloc` 返回 **`void *`（虚拟地址）**，内部就是「物理分配 + `__va()` 转虚拟 + 清零」。**物理地址**给「还没建映射、只能按物理地址操作」的场景（如页表项、memmap 描述）；**虚拟地址**给「要直接读写内存」的调用者（`memset` 就得用虚拟地址）。

**Q2：为什么 `memblock_alloc` 要 `memset(ptr, 0, size)` 清零？**
A：启动期的分配对象是页表、`struct page` 数组这类**元数据**，如果不清零，残留的脏数据会导致**页表里有垃圾项、`struct page` 里是随机字段**——后果是「未初始化的内存被当作有效状态」。启动期清零成本低（就一次），换来的确定性很高。

**Q3：早期 `memblock_free` 为什么「几乎没用」？**
A：因为此时 **`struct page` 还没建好**，没有 per-page 元数据，也就无法把这些页挂进 Buddy 的 freelist。早期 free 只是**记账**（从 `reserved` 表移除），真正「还给系统」要等 §4 的 `memblock_free_all()`。所以启动期「分配后几乎不 free」——反正 free 了也进不了 Buddy。

**Q4：`align` 参数对页表分配为什么关键？**
A：硬件页表要求**物理地址按页大小对齐**——4K 页表要 4K 对齐，2M 大页要 2M 对齐，1G 大页要 1G 对齐。`memblock_alloc(size, align)` 的 `align` 就是保证「返回的物理地址是 align 的整数倍」，否则 MMU 根本不认这段地址当页表。

**Q5：`memblock_alloc_try_nid` 里的 `min_addr`/`max_addr` 和 `nid` 分别约束什么？**
A：`min_addr`/`max_addr` 约束**物理地址窗口**（如 `memblock_alloc_low` 就设 `max_addr` 为低端上限，保证 DMA 可达）；`nid` 约束**NUMA 节点**（`NUMA_NO_NODE` = 不限制）。两者是正交的两个维度：**「在哪段地址」** vs **「在哪个节点」**。

</details>

---
