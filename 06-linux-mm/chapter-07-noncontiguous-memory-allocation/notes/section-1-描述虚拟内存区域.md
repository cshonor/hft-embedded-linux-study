# Ch 7 §1 描述虚拟内存区域 (`vm_struct` → `vmap_area`)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**
> 源码核验：Linux **v6.6**（`include/linux/vmalloc.h` / `mm/vmalloc.c`）

---

## 本节讲什么

本节回答：**内核用什么数据结构描述一段 vmalloc 区域？**

原书答案是 `vm_struct`——**一个单链表**，每个节点描述一段非连续分配的内核虚拟区间。但 **v6.6 里已经升级成「双结构 + 双索引」**：`vm_struct`（区域描述符）+ `vmap_area`（地址区间，同时挂在一棵**红黑树**和一条**双向链表**上）。本节先讲原书链表方案，再落到 v6.6 真身——这是理解「为什么现代 vmalloc 查找/释放是 O(log n)」的关键。

---

## 1. 内核 vmalloc 区：`VMALLOC_START` ~ `VMALLOC_END`

内核在**虚拟地址空间**里划了一块专用窗口给非连续分配：

```
              内核虚拟地址空间（x86_64，示意）
    ┌──────────────────────────────────────┐ 高地址
    │  MODULES_VADDR  (模块区，独立或并入)   │
    ├──────────────────────────────────────┤
    │  VMALLOC_START ═══════════════════    │
    │   vmap_area #1  [ vm_struct A ]      │
    │   (guard page)                       │  ◄─ 非连续分配窗口
    │   vmap_area #2  [ vm_struct B ]      │
    │   ...                                │
    │  VMALLOC_END   ═══════════════════    │
    ├──────────────────────────────────────┤
    │  ... 固定映射 / vsyscall / 栈 ...      │
    ├──────────────────────────────────────┤
    │  PAGE_OFFSET (直接映射区，__va/__pa)   │
    └──────────────────────────────────────┘ 低地址
```

| 概念 | 说明 |
|------|------|
| **`VMALLOC_START` ~ `VMALLOC_END`** | 非连续分配专用的**内核虚拟地址窗口**，与直接映射区（`PAGE_OFFSET`，物理地址+固定偏移）**分离** |
| **`is_vmalloc_addr()`** | `mm/vmalloc.c:82` —— `return addr >= VMALLOC_START && addr < VMALLOC_END`，判断一个地址是否落在 vmalloc 区（KASAN 标签要先 `kasan_reset_tag` 抹掉） |
| **`vm_struct`** | 描述**一段 vmalloc 区域**的元数据——**不是**用户态的 `vm_area_struct`（VMA，Ch4） |
| **`vmap_area`** | 描述**虚拟地址区间本身**，是红黑树/链表里的索引节点 |

**关键直觉：`vm_struct` 管「这段区域是什么」**（映射了多少页、哪个调用者、什么用途），**`vmap_area` 管「这段地址在哪」**（起始/结束地址，参与地址排序查找）。原书只有前者，v6.6 把「地址索引」职责拆给了后者。

---

## 2. 原书方案：`vm_struct` 单链表

原书（2.4/2.6 早期）用一条**全局单链表**串起所有 `vm_struct`：

```
vm_struct_list (全局单链表)
    head ──► vm_struct A ──► vm_struct B ──► ... ──► NULL
              (addr,size)     (addr,size)
```

- **分配**：从头到尾扫链表，找一个「足够大且不与现有区间重叠」的空隙 → O(n)。
- **释放**：扫链表找到包含 `addr` 的节点 → O(n)。
- **`next` 字段**：单链表指针。

**本质缺陷**：所有查找都是**线性遍历**。区域少时无所谓，但 vmalloc 区里塞满数百个模块/驱动映射时，每次 `vfree`/`vmalloc` 都要扫全表——这就是 v6.6 引入红黑树的动机。

---

## 3. v6.6 真身：`vm_struct` + `vmap_area` 双索引

v6.6 里，`vm_struct` 仍然存在，但它从「既是描述符又是索引节点」退化为**纯描述符**；地址索引的活儿交给了 `vmap_area`（`include/linux/vmalloc.h`）：

```c
/* include/linux/vmalloc.h */
struct vm_struct {                       /* :49  区域描述符 */
    struct vm_struct *next;              /* :50  链表指针（仍保留，见下） */
    void            *addr;               /* :51  区域起始虚拟地址 */
    unsigned long    size;               /* :52  大小（含 guard page） */
    unsigned long    flags;              /* :53  VM_* 标志（见 §1.4） */
    struct page     **pages;             /* :54  物理页指针数组 */
#ifdef CONFIG_HAVE_ARCH_HUGE_VMALLOC
    unsigned int     page_order;         /* :56  大页阶数（huge vmalloc） */
#endif
    unsigned int     nr_pages;           /* :58  页数 */
    phys_addr_t      phys_addr;          /* :59  物理地址（ioremap 时用） */
    const void      *caller;             /* :60  调用者返回地址（调试/统计） */
};

struct vmap_area {                       /* :63  地址区间 + 索引节点 */
    unsigned long va_start;              /* :64  区间起始地址 */
    unsigned long va_end;                /* :65  区间结束地址 */

    struct rb_node rb_node;              /* :67  按地址排序的红黑树节点 */
    struct list_head list;               /* :68  按地址排序的双向链表节点 */

    /*
     * vmap_area 同一时刻只属于两棵树之一：
     *   1) "free" 树（根 free_vmap_area_root）
     *   2) "busy" 树（根 vmap_area_root）
     */
    union {                              /* :76  两棵树复用同一存储 */
        unsigned long subtree_max_size;  /* :77  free 树：子树最大空闲区间 */
        struct vm_struct *vm;            /* :78  busy 树：指向 vm_struct */
    };
    unsigned long flags;                 /* :80  vm_map_ram 区域类型标记 */
};
```

### 字段速查：`vm_struct`（`vmalloc.h:49-61`）

| 字段 | 作用 |
|------|------|
| `next` | 链表指针。v6.6 里它仍在，但**主索引已换成 `vmap_area` 的红黑树**；这条链表主要用于早期启动（`vm_area_add_early`）与部分遍历 |
| `addr` | 区域起始虚拟地址（`get_vm_area` 返回、`vfree` 传入的入口） |
| `size` | 区域大小。**注意含 guard page**，取「真实可用大小」要用 `get_vm_area_size()`（`vmalloc.h:196`，非 `VM_NO_GUARD` 时减一页） |
| `flags` | `VM_*` 标志位（见 §1.4 表） |
| `pages` | 指向 `struct page *` 数组。数组本身可能用 `kmalloc`（≤1 页）或 `__vmalloc_node`（>1 页）分配 |
| `page_order` | 大页阶数，仅 `CONFIG_HAVE_ARCH_HUGE_VMALLOC` 存在；0 表示普通 4K 页 |
| `nr_pages` | 已分配物理页数量（`size >> PAGE_SHIFT`） |
| `phys_addr` | `ioremap` 类映射（`VM_IOREMAP`）的**目标物理基址**；纯 `vmalloc` 时为 0 |
| `caller` | 调用者的返回地址（`__builtin_return_address(0)`），用于 `/proc/vmallocinfo` 溯源 |

### 字段速查：`vmap_area`（`vmalloc.h:63-81`）

| 字段 | 作用 |
|------|------|
| `va_start` / `va_end` | 区间 `[start, end)` 的虚拟地址范围 |
| `rb_node` | 红黑树节点，**按地址排序**——查「哪个区间覆盖某地址」O(log n) |
| `list` | 双向链表节点，同样**按地址排序**——顺序遍历整个 vmalloc 区时用 |
| `union` | **同一时刻只属于一棵树**：free 树里存 `subtree_max_size`（子树内最大空闲区间，用于快速找「足够大的洞」），busy 树里存 `vm`（指向已映射的 `vm_struct`） |
| `flags` | 标记 `vm_map_ram` 区域的类型（`VMAP_RAM` / `VMAP_BLOCK` 等） |

### 双索引：free 树 + busy 树

```
        free_vmap_area_root (红黑树)          vmap_area_root (红黑树)
        ┌──────────────────────┐              ┌──────────────────────┐
        │  subtree_max_size    │              │  vm_struct *vm       │
        │  每个节点记「子树里     │              │  每个节点指向已映射的  │
        │   最大的空闲洞」       │              │  区域描述符           │
        └──────────────────────┘              └──────────────────────┘
                ↑ 分配时查这里                        ↑ 释放/查找时查这里
             (alloc_vmap_area)                  (find_unlink_vmap_area)

        另有 vmap_area_list 双向链表贯穿全部（含 free + busy），供顺序遍历
```

- **分配**：在 **free 树**里用 `subtree_max_size` 剪枝，O(log n) 找到足够大的洞（`__alloc_vmap_area`，`mm/vmalloc.c:1489`）。
- **释放**：把节点从 **busy 树**摘下来，插回 **free 树**（`free_vmap_area`，`:1538`）。
- **查找**（如 `vfree` 定位区间）：在 **busy 树**里按地址 O(log n) 找到 `vmap_area`，再取 `union.vm` 拿到 `vm_struct`。

**为什么 union 省内存？** 一个 `vmap_area` 要么空闲要么在用，`subtree_max_size` 和 `vm` 指针**永远不会同时需要**，共享 8 字节存储，把红黑树节点 + 链表节点 + 附加字段压进一个紧凑结构。

---

## 4. `VM_*` 标志（`vmalloc.h:20-30`）

| 标志 | 值 | 含义 |
|------|----|------|
| `VM_IOREMAP` | `0x00000001` | `ioremap()` 类 I/O 映射 |
| `VM_ALLOC` | `0x00000002` | `vmalloc()` 分配 |
| `VM_MAP` | `0x00000004` | `vmap()` 映射的页（页由调用者提供） |
| `VM_USERMAP` | `0x00000008` | 可通过 `remap_vmalloc_range()` 映射到用户态 |
| `VM_DMA_COHERENT` | `0x00000010` | `dma_alloc_coherent()` 分配 |
| `VM_UNINITIALIZED` | `0x00000020` | `vm_struct` **尚未完全初始化**（分配中途标记，完成后 `clear_vm_uninitialized_flag` 清掉） |
| `VM_NO_GUARD` | `0x00000040` | ⚠️ **不加 guard page**（危险，见 §1.5） |
| `VM_KASAN` | `0x00000080` | 已分配 KASAN shadow 内存 |
| `VM_FLUSH_RESET_PERMS` | `0x00000100` | unmap 时重置 direct map 权限并 flush TLB（不可原子上下文释放） |
| `VM_MAP_PUT_PAGES` | `0x00000200` | `vfree` 时**释放 pages 数组本身**（`vmap()` 转移所有权时置位） |
| `VM_ALLOW_HUGE_VMAP` | `0x00000400` | 允许在支持 `HAVE_ARCH_HUGE_VMALLOC` 的架构上做大页映射 |

`VM_NO_GUARD` 和 `VM_ALLOW_HUGE_VMAP` 是 v6.6 里最值得注意的两个：前者关系到越界安全，后者关系到 TLB 性能（§4 详述）。

---

## 5. Guard page：相邻区域的隔离带

`__get_vm_area_node`（`mm/vmalloc.c:2570`）里有一段：

```c
if (!(flags & VM_NO_GUARD))        /* :2592 默认加保护页 */
    size += PAGE_SIZE;             /* :2593 请求大小额外 +1 页 */
```

**默认每段 vmalloc 区域末尾多占一个未映射页（guard page）**，作用：

```
    vmap_area A           guard            vmap_area B
    [addr  ...  end]      [未映射 1 页]     [addr  ...  end]
              ↑ 越界写到这里会触发缺页，而不是悄悄改坏 B
```

| 要点 | 说明 |
|------|------|
| **防越界** | 写越界时踩到 guard page → 页错误，立刻暴露 bug；否则会**悄悄破坏相邻区域**，事后极难排查 |
| **代价** | 每段多占一页虚拟地址 + 可能一页页表开销；`get_vm_area_size()`（`:196`）返回真实大小时要**把这一页减掉** |
| **豁免** | `VM_NO_GUARD` 显式去掉（如 `vmap()` 内部会警告并强制恢复，见 `:2910` "Your top guard is someone else's bottom guard"） |

**这段注释很精髓**：`vmap()` 里即使调用者传了 `VM_NO_GUARD`，也会 `WARN_ON_ONCE` 后强制 `flags &= ~VM_NO_GUARD`——因为「你的顶部 guard 是别人底部的 guard」，省掉自己这页会连带破坏相邻区域的隔离。

---

## 6. HFT / 嵌入式关联

| 场景 | 关联 |
|------|------|
| **vmalloc 区遍历** | 驱动模块多时，`vmap_area` 红黑树把 `vfree`/`find_vm_area` 从 O(n) 压到 O(log n)——**释放热路径不随区域数线性变慢** |
| **`/proc/vmallocinfo` 溯源** | 每个 `vm_struct->caller` 记录了「谁分配的」，排查内核内存泄漏时直接定位到函数级（`mm/vmalloc.c:4443` 注册的 seq 接口） |
| **guard page 的启发** | HFT 里用户态 `mmap` 相邻映射也应留隔离带（`MAP_FIXED` 时不慎相邻），思路同源 |
| **direct map vs vmalloc 区** | 理解「物理连续（直接映射） vs 仅虚拟连续（vmalloc）」的**第一性区别**——后者每页都要过页表，TLB 覆盖更差（§2/§4 展开） |

---

## 7. 衔接

- 下节 [§2 分配非连续区域](./section-2-分配非连续区域.md)：free 树怎么被 `alloc_vmap_area` 用起来、物理页怎么批量分配
- 用户态对应物：[Ch4 §3 内存区域](../../chapter-04-process-address-space/notes/section-3-内存区域.md)（`vm_area_struct`，两套描述符别混）
- 页表本身：[Ch3 §1 页目录与页表项](../../chapter-03-page-table-management/notes/section-1-页目录与页表项.md)

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：`vm_struct` 和 `vmap_area` 的分工是什么？为什么 v6.6 要拆成两个？**
A：`vm_struct` 管「这段区域**是什么**」——映射了多少页、哪个调用者、什么 `VM_*` 用途；`vmap_area` 管「这段**地址在哪**」——`va_start/va_end` 区间，并作为红黑树/链表节点参与地址排序。原书只有 `vm_struct` 单链表，查找/释放都是 O(n)；拆出 `vmap_area` 后地址索引落到红黑树上，查找 O(log n)。本质是**把「描述符」和「索引」两个职责解耦**。

**Q2：`vmap_area` 的 union 为什么能同时存 `subtree_max_size` 和 `vm_struct *`？**
A：因为一个 `vmap_area` **同一时刻只属于一棵树**：空闲时挂在 `free_vmap_area_root`（需要 `subtree_max_size` 记录子树最大洞，供快速找洞），已映射时挂在 `vmap_area_root`（需要 `vm` 指针指回描述符）。两个字段互斥使用，共享同一段存储省 8 字节。

**Q3：`vm_struct->size` 为什么和「真实可用大小」对不上？**
A：默认分配时 `__get_vm_area_node` 会 `size += PAGE_SIZE` 塞一个 guard page（除非 `VM_NO_GUARD`）。所以 `size` 包含保护页，真实可用大小要用 `get_vm_area_size()`（非 `VM_NO_GUARD` 时减一页）。这个保护页用于**把越界写暴露成缺页**，而非悄悄破坏相邻区域。

**Q4：`VM_MAP_PUT_PAGES` 标志解决什么问题？**
A：`vmap()` 时调用者传入自己准备好的 `pages` 数组；若置 `VM_MAP_PUT_PAGES`，则**数组所有权**（连同每页一个引用）转移给 vmalloc 层，后续 `vfree()` 会一并释放数组 + 每页引用。否则 `vmap` 只负责映射，页和数组仍归调用者管（用 `vunmap` 而非 `vfree` 释放）。这区分了「谁持有物理页」。

**Q5：`find_vm_area()` 在 v6.6 里怎么实现？和原书单链表扫描有什么不同？**
A：`find_vm_area()`（`:2663`）调 `find_vmap_area()` 在 **busy 红黑树**里按地址 O(log n) 找到 `vmap_area`，再返回 `va->vm`。原书是沿单链表线性扫描，找到第一个覆盖地址的节点。两者结果相同，但复杂度从 O(n) 降到 O(log n)。

</details>
