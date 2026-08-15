# 虚拟内存与地址空间 (VMA红黑树 → maple tree)

> 笨叔《奔跑吧 Linux 内核》读书笔记
> 对应旧书: ULK3 / LKD3 (Linux 2.6)
> 对应现代内核: Linux 5.x / 6.x

---

## 本节要点

### 进程地址空间管理演进

Linux 用 VMA (Virtual Memory Area) 描述进程的虚拟地址区间。VMA 的组织结构经历了重大变化：

| 时期 | 数据结构 | 问题 |
|------|---------|------|
| 2.6 ~ 5.x | 红黑树 + 链表 | 大量 VMA 时查找 O(log N)，但锁竞争严重 |
| 6.1+ | **Maple Tree** | RCU-safe 读取，无需 mmap_sem 读取锁 |

### Maple Tree（6.1+）

Maple Tree 是一种 B-tree 变体，专门为 VMA 管理设计：

```c
// 旧 API (需要 mmap_sem 读锁)
struct vm_area_struct *vma = find_vma(mm, addr);
// 需要 down_read(&mm->mmap_sem) 保护

// 新 API (RCU-safe, 6.1+)
struct vm_area_struct *vma = find_vma(mm, addr);
// 内部使用 maple tree, RCU 读取无需锁
```

**Maple Tree 的关键优势：**
- **RCU-safe 读取**：查找 VMA 不需要 `mmap_lock`（原 `mmap_sem`），多线程并发读取无竞争
- **更好的缓存局部性**：节点内部连续存储，减少 cache miss
- **并发性**：写操作仍然需要锁，但读操作可以与写操作并发

### mmap_lock（原 mmap_sem）

```c
// 5.x: struct rw_semaphore mmap_sem
down_read(&mm->mmap_sem);    // 读锁
// ... find_vma, page fault handling ...
up_read(&mm->mmap_sem);

// 6.x: struct rw_semaphore mmap_lock (改名)
mmap_read_lock(mm);          // 读锁 (maple tree 查找仍建议加锁)
mmap_read_unlock(mm);

mmap_write_lock(mm);         // 写锁 (mmap, munmap, brk 等)
mmap_write_unlock(mm);
```

### 页表结构

```
虚拟地址 (64-bit)
┌──────────┬──────────┬──────────┬──────────┐
│ PGD (9b) │ PUD (9b) │ PMD (9b) │ PTE (9b) │ 页内偏移 (12b)
└──────────┴──────────┴──────────┴──────────┘
     │          │          │          │
  pgd_offset  pud_offset  pmd_offset  pte_offset
     │          │          │          │
  4KB       4KB         4KB        4KB = 物理页

5 级页表 (57-bit VA, CONFIG_PGTABLE_LEVELS=5):
PGD → P4D → PUD → PMD → PTE
```

### 关键 VMA 操作

```c
// 源码路径: mm/mmap.c
// 创建 VMA
unsigned long mmap(struct file *file, unsigned long addr,
                   unsigned long len, int prot, int flags,
                   unsigned long offset);

// 6.x 内部实现使用 maple tree
// mm/mmap.c: do_mmap() → maple_tree_insert()
```

---

## 与旧书对比

| ULK3 / LKD3 (2.6) | 笨叔 (5.x/6.x) | 变化原因 |
|--------------------|-----------------|----------|
| VMA 红黑树 + 链表 | Maple Tree (6.1+) | 读操作 RCU-safe，减少 mmap_lock 竞争 |
| `mmap_sem` | `mmap_lock` (5.8 改名) | 语义更清晰 |
| 4 级页表 (48-bit VA) | 5 级页表 (57-bit VA, 可选) | 大内存服务器需要 >128TB |
| `find_vma()` 需要锁 | `find_vma()` RCU-safe | Maple Tree 支持无锁读 |
| `vm_area_struct` 有 `vm_rb` | `vm_rb` 字段删除 | 不再用红黑树 |

---

## 关键数据结构 / 函数

```c
// 源码路径: include/linux/mm_types.h
struct vm_area_struct {
    unsigned long vm_start;      // 起始虚拟地址
    unsigned long vm_end;        // 结束虚拟地址
    struct mm_struct *vm_mm;     // 所属 mm
    pgprot_t vm_page_prot;       // 访问权限
    unsigned long vm_flags;      // VM_READ/WRITE/EXEC/SHARED
    struct file *vm_file;        // 映射的文件 (如有)
    // 6.1+: 不再有 struct rb_node vm_rb;
    // 改为 maple tree 管理
};

// 源码路径: include/linux/maple_tree.h
struct maple_tree {
    void *ma_root;               // 根节点
    unsigned int ma_flags;
    spinlock_t ma_lock;
};

// 源码路径: include/linux/mm_types.h
struct mm_struct {
    struct maple_tree mm_mt;     // VMA maple tree (6.1+, 替代 rb_root)
    // struct rb_root mm_rb;     // 已删除
    struct rw_semaphore mmap_lock;  // 5.8+ 改名 (原 mmap_sem)
};
```

---

## HFT 关联

- **mmap_lock 竞争**：多线程交易进程频繁 page fault 时，`mmap_sem` 读锁竞争导致尾延迟毛刺。Maple Tree 的 RCU-safe 读取直接解决这个问题
- **大页 mmap**：HFT 用 `mmap(MAP_HUGETLB)` 预分配大页，减少 page fault 次数和 TLB miss
- **NUMA 页表本地化**：页表本身也占用内存，跨 NUMA 节点访问页表增加延迟。`numactl --cpunodebind` 确保页表在本地节点
- **mlockall**：HFT 启动后调 `mlockall(MCL_CURRENT|MCL_FUTURE)` 锁定所有页，防止 swap 导致微秒级停顿

---

## 自测

<details>
<summary>Q1: Maple Tree 相比红黑树在 VMA 管理上的核心优势是什么？</summary>

核心优势是 **RCU-safe 读取**。红黑树查找 VMA 需要 `mmap_lock` 读锁，多线程并发访问时锁竞争导致性能下降。Maple Tree 支持 RCU 无锁读取，查找 VMA 时不需要获取 `mmap_lock`，显著减少多线程进程的锁竞争。这对多线程应用（如 HFT 交易引擎多线程共享地址空间）尤为重要。
</details>

<details>
<summary>Q2: 5 级页表什么时候启用？对 HFT 有什么影响？</summary>

5 级页表在 `CONFIG_PGTABLE_LEVELS=5` 且 CPU 支持 LA57（57 位虚拟地址）时启用，支持 128PB 虚拟地址空间。对 HFT 的影响：(1) 多一级页表意味着多一次内存访问（页表 walk），TLB miss 代价更高；(2) 但 HFT 通常虚拟地址空间不大，TLB 命中率高；(3) 实际 HFT 系统 通常不启用 5 级页表，4 级 (48-bit) 足够。
</details>

<details>
<summary>Q3: 为什么 HFT 要调用 mlockall？它和 MAP_HUGETLB 有什么配合关系？</summary>

`mlockall(MCL_CURRENT|MCL_FUTURE)` 锁定当前和未来所有页在物理内存中，禁止 swap。HFT 延迟敏感，swap 会导致毫秒级停顿。配合 `MAP_HUGETLB`：(1) 大页减少 page fault 次数（2MB 大页 = 512 个 4KB 页合一）；(2) 大页减少 TLB miss；(3) `mlockall` 确保大页不被回收。典型流程：启动时 `mmap(MAP_HUGETLB|MAP_LOCKED)` 预分配大页内存池 → `mlockall` 锁定。
</details>
