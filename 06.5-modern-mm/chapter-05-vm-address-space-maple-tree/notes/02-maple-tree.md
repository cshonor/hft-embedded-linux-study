# Maple Tree (6.1+)

> **原文:** [The maple tree data structure](https://lwn.net/Articles/845507/) (LWN, 2021)
> **作者:** Liam Howlett
> **内核版本:** 6.1+ (替换 VMA 红黑树)
> **对标旧书:** ULK3 Ch9 / LKD3 Ch15 (VMA 红黑树管理)
> 🔗 **LKD 章节语境（本书精读笔记）**：[Ch6.6 选择合适的数据结构](../../../05-linux-kernel/chapter-06-kernel-data-structures/notes/section-6.6-选择合适的数据结构.md)
> （B-Tree vs rbtree 的层数对比 + 指针低位打标）· [Ch15 进程地址空间](../../../05-linux-kernel/chapter-15-process-address-space/README.md)

---

## 核心观点

Maple Tree 是为 VMA 管理设计的 B-tree 变体，替换了 Linux 使用 20 年的红黑树。

### 红黑树的问题

```c
// 旧: VMA 用红黑树组织
struct mm_struct {
    struct rb_root mm_rb;           // 红黑树根
    struct vm_area_struct *mmap;    // 链表
};

// find_vma() 需要锁保护
struct vm_area_struct *find_vma(struct mm_struct *mm, unsigned long addr)
{
    struct rb_node *rb_node;
    // 需要 down_read(&mm->mmap_sem) — 多线程竞争!
    rb_node = mm->mm_rb.rb_node;
    while (rb_node) {
        vma = rb_entry(rb_node, struct vm_area_struct, vm_rb);
        if (addr < vma->vm_end)
            rb_node = rb_node->rb_left;
        else if (addr >= vma->vm_end)
            rb_node = rb_node->rb_right;
        else
            return vma;
    }
    return NULL;
}
```

问题：(1) 多线程同时 `find_vma()` 需要 `mmap_sem` 读锁竞争；(2) 红黑树节点分散在内存中，cache miss 多。

### Maple Tree 设计

```
Maple Tree (B-tree 变体):
  根节点 (最多 16 个 pivot)
  ├── [0, 0x4000): 指向 VMA A
  ├── [0x4000, 0x8000): 指向 VMA B
  ├── [0x8000, 0xC000): 指向子节点
  │     ├── [0x8000, 0xA000): VMA C
  │     └── [0xA000, 0xC000): VMA D
  └── ...
```

**关键优势：**
- **RCU-safe 读取**：`find_vma()` 无需 `mmap_lock` 读锁
- **缓存友好**：节点内部连续存储 pivot + pointer，一次 cache line 读取多个 VMA
- **并发性**：读写可并发（写操作用 spinlock，读操作用 RCU）

### 性能对比

| 操作 | 红黑树 | Maple Tree |
|------|--------|-----------|
| find_vma (读) | O(log N), 需锁 | O(log N), RCU 无锁 |
| mmap (写) | O(log N) | O(log N) |
| 遍历 VMA | O(N) 链表 | O(N) 树遍历 |
| 多线程竞争 | 锁竞争严重 | 读无竞争 |

---

## 与旧书差异

| ULK3 / LKD3 讲的 | 现代实现 |
|-------------------|---------|
| VMA 红黑树 (`mm_rb`) | Maple Tree (`mm_mt`) |
| `find_vma()` 需 `mmap_sem` | `find_vma()` RCU-safe |
| `vm_area_struct.vm_rb` | 字段删除 |
| `mmap_sem` | `mmap_lock` (5.8 改名) |

---

## HFT 关联

多线程交易进程（多线程共享 mm_struct）频繁 `find_vma()` 时，红黑树的 `mmap_sem` 读锁竞争导致尾延迟毛刺。Maple Tree 的 RCU 无锁读取直接消除这个问题。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** Maple Tree 为什么能实现 RCU-safe 读取而红黑树不能？

> 红黑树的旋转操作会改变节点间父子关系，读取者可能看到中间状态（如节点被旋转但父指针未更新），导致访问已释放节点。Maple Tree 的写操作是 copy-on-write：修改时复制节点，更新指针（原子写），旧节点通过 RCU 延迟释放。读取者要么看到旧节点要么看到新节点，不会看到中间状态。

**Q2:** Maple Tree 的 pivot 是什么概念？

> pivot 是区间端点。每个节点存储多个 pivot 值，将地址空间划分为区间。例如节点有 pivot [0x4000, 0x8000, 0xC000]，表示四个区间：[0, 0x4000)、[0x4000, 0x8000)、[0x8000, 0xC000)、[0xC000, MAX)。每个区间对应一个子指针或 VMA 指针。这种设计让一个节点覆盖多个 VMA，减少树高度和 cache miss。

</details>
