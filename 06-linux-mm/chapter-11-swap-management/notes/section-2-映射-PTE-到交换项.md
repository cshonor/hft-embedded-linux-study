# Ch 11 §2 映射 PTE 到交换项 (PTE ↔ Swap Entry)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**
> 源码核验：Linux **v6.6**（`include/linux/swapops.h` / `include/linux/swap.h`）

---

## 本节讲什么

本节回答：**页换出后，PTE 里存什么？内核怎么从 PTE 反查到"数据在磁盘哪"？**

核心是 `swp_entry_t`——一个**复用 PTE 位域**编码 `{type, offset}` 的小结构。原书讲了这个概念，v6.6 里它的**位域布局**更精确了：`type` 放高位（5 bit）、`offset` 右对齐低位，且区分"架构相关格式"与"架构无关格式"。本节落到源码。

---

## 1. 问题：换出后 PFN 失效

```
换出前：PTE → present=1 → PFN → struct page → 物理页
换出后：物理页框已归还 Buddy，PFN 字段不再有意义
        └─ 必须复用 PTE 的位域，改存「磁盘位置」
```

页一旦换出，`struct page` 被回收，原来 PTE 里存的 PFN 就**失效**了。Linux 的做法是：把 PTE 标记为 **not present**，然后把原本放 PFN 的位域**改存 `swp_entry_t`**——一个指向 swap 区里某个 slot 的"指针"。

---

## 2. `swp_entry_t`：位域布局（`swapops.h:27-40`）

```c
/* include/linux/swapops.h */
#define SWP_TYPE_SHIFT	(BITS_PER_XA_VALUE - MAX_SWAPFILES_SHIFT)  /* :27 */
#define SWP_OFFSET_MASK	((1UL << SWP_TYPE_SHIFT) - 1)              /* :28 */
```

```
swp_entry_t.val 的位布局（64 位）：
  ┌─────────────────────────┬──────────────────────────────────┐
  │   type（高 5 bit）        │   offset（低 59 bit，右对齐）       │
  └─────────────────────────┴──────────────────────────────────┘
   ↑ SWP_TYPE_SHIFT = 59      ↑ SWP_OFFSET_MASK = (1<<59)-1
```

| 字段 | 位置 | 含义 |
|------|------|------|
| **`type`** | 高 5 bit（`MAX_SWAPFILES_SHIFT=5`） | `swap_info[]` 数组下标——哪个 swap 区 |
| **`offset`** | 低 59 bit（右对齐） | 该 swap 区内的 **slot 编号**（页号） |

**关键设计**：`offset` 占绝大部分位（59 bit），意味着**单个 swap 区可以有 2⁵⁹ 个 slot**——足够大。而 `type` 只占 5 bit，所以 swap 区数量上限才被卡在 32（§1）。

---

## 3. 打包/解包：三个宏（`swapops.h:86-107`）

```c
/* 打包：type + offset → swp_entry_t */
static inline swp_entry_t swp_entry(unsigned long type, pgoff_t offset)
{
    swp_entry_t ret;
    ret.val = (type << SWP_TYPE_SHIFT) | (offset & SWP_OFFSET_MASK);  /* :90 */
    return ret;
}

/* 解包：取出 type */
static inline unsigned swp_type(swp_entry_t entry)
{
    return (entry.val >> SWP_TYPE_SHIFT);                             /* :100 */
}

/* 解包：取出 offset */
static inline pgoff_t swp_offset(swp_entry_t entry)
{
    return (entry.val & SWP_OFFSET_MASK);                             /* :107 */
}
```

三个宏就是**位运算**：打包 = 左移 OR，解包 = 右移 / 掩码。没有运行时开销，编译器直接优化成常数操作。

---

## 4. 架构相关 ↔ 架构无关格式

```c
/* PTE（架构相关格式）↔ swp_entry_t（架构无关格式） */
static inline swp_entry_t pte_to_swp_entry(pte_t pte)       /* :133 */
{
    swp_entry_t arch_entry = __pte_to_swp_entry(pte);       /* 架构相关解码 */
    return swp_entry(__swp_type(arch_entry), __swp_offset(arch_entry));
}

static inline pte_t swp_entry_to_pte(swp_entry_t entry)     /* :146 */
{
    swp_entry_t arch_entry = __swp_entry(swp_type(entry), swp_offset(entry));
    return __swp_entry_to_pte(arch_entry);                  /* 架构相关编码 */
}
```

| 格式 | 说明 |
|------|------|
| **架构无关 `swp_entry_t`** | 内核逻辑层用的统一格式，`type`/`offset` 固定布局 |
| **架构相关 PTE** | 各架构（x86/arm64）在 PTE 位域里的**实际摆法**，由 `__pte_to_swp_entry`/`__swp_entry_to_pte` 转换 |

注释 `:25` 强调：`swp_entry_t` **从不以架构相关格式存储**，只在进出 PTE 的边界做一次转换——这隔离了各架构 PTE 位域布局的差异。

---

## 5. folio 时代的 swap entry：`folio->swap`

v6.6 里 swap entry 不再只活在 PTE 里，`struct folio` 也存了一份：

```c
/* include/linux/swap.h:336 */
static inline swp_entry_t page_swap_entry(struct page *page)
{
    struct folio *folio = page_folio(page);
    swp_entry_t entry = folio->swap;          /* folio 里直接存 swap entry */
    entry.val += folio_page_idx(folio, page); /* 大页时加上页内偏移 */
    return entry;
}
```

**为什么 folio 也要存？** PTE 是"读者视角"（进程怎么找到这个页），`folio->swap` 是"页本身视角"（这个页当前被换到哪了）。两者互补：swap cache 里查页、`try_to_unuse` 反查 PTE、`delete_from_swap_cache` 清理时，都需要"页 → swap entry"的映射，不必每次去反解 PTE。

---

## 6. 完整往返：换出 → 换入

```
换出 (swap out):
  1. 分配 swap slot → swp_entry(type, offset)
  2. folio->swap = entry（页记录自己的去向）
  3. 写 swap
  4. rmap 把每个映射此页的 PTE 改写成 !present + swp_entry_to_pte(entry)

换入 (swap in):
  1. 进程访问 VA → 缺页 fault
  2. 内核看 PTE：present=0 且是 swap entry → pte_to_swp_entry 解出 type/offset
  3. 从 swap_info[type] 找到 swap 区 → 读 offset 处数据
  4. 分配新物理页 → 数据填回 → PTE 改回 present + 新 PFN
```

**PTE 的 present 位是分水岭**：present=1 表示"在 RAM，看 PFN"；present=0 表示"不在 RAM，看 swap entry"。

---

## 7. HFT / 嵌入式关联

| 场景 | 关联 |
|------|------|
| **位域复用的启示** | PTE 位域在 present/swap 两态间复用，是"**一个字段两种语义**"的经典手法——HFT 里网络包头的 tag 位、对象头的状态位都是同构设计 |
| **`pgmajfault` 监控** | 换入（swap in fault）在 vmstat 里计入 `pgmajfault`（major fault）——这是**最贵的缺页类型**，HFT 必须归零 |
| **架构无关格式** | "逻辑格式 ↔ 物理格式"分离让 x86/arm64 共用 swap 核心逻辑——可移植性的标准手法 |

---

## 8. 衔接

- 下节 [§3 分配交换槽](./section-3-分配交换槽.md)：`swp_entry(type, offset)` 的 `offset` 是怎么分配的
- PTE 位域：[Ch3 §1 页目录与页表项](../../chapter-03-page-table-management/notes/section-1-页目录与页表项.md)

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：`swp_entry_t` 里 `type` 和 `offset` 各占多少位？为什么这样分配？**
A：`type` 占高 5 bit（`MAX_SWAPFILES_SHIFT=5`），`offset` 占低 59 bit（右对齐）。这样分配的原因：① `type` 只区分"哪个 swap 区"，32 种足够；② `offset` 需要表达"区内的 slot 号"，越大越好，所以把绝大部分位让给它。这个布局决定了"swap 区数量受限、单个区容量几乎无限"的非对称性。

**Q2：`swp_type()` 和 `swp_offset()` 的实现是什么？为什么没有运行时开销？**
A：`swp_type` = `entry.val >> SWP_TYPE_SHIFT`（右移取高位），`swp_offset` = `entry.val & SWP_OFFSET_MASK`（掩码取低位）。都是纯位运算，编译器会内联成常数移位/掩码，零运行时开销。

**Q3：为什么说 `swp_entry_t` "从不以架构相关格式存储"？**
A：各架构（x86/arm64）在 PTE 位域里的摆法不同。内核只在 PTE 边界用 `__pte_to_swp_entry`/`__swp_entry_to_pte` 做一次转换，其余逻辑层（swap cache、slot 分配、`folio->swap`）统一用架构无关的 `swp_entry_t`。这隔离了架构差异，swap 核心逻辑可以跨架构复用。

**Q4：folio 为什么要存一份 swap entry（`folio->swap`），而不只靠 PTE？**
A：因为 PTE 是"进程视角"，`folio->swap` 是"页本身视角"。很多操作需要"从页反查它在哪"而**不能**反解 PTE：swap cache 查页、`delete_from_swap_cache` 清理、`try_to_unuse` 遍历时定位 slot。存一份能让这些路径直接拿，不必扫描所有进程页表。

**Q5：PTE 的 present 位在 swap 场景里起什么作用？**
A：它是**分水岭**。present=1 → PTE 其余位是 PFN（页在 RAM）；present=0 且是 swap entry → PTE 其余位是 `swp_entry_t`（页在 swap）。缺页处理时，内核先看 present 位：置 1 走普通缺页，置 0 且是 swap entry 则走换入路径（swap in）。

</details>
