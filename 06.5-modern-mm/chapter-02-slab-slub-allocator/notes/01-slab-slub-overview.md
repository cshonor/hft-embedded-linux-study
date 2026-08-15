# Bootlin: Slab/SLUB 分配器

> **来源:** [Bootlin Kernel Training — Memory Management](https://bootlin.com/docs/kernel/)
> **主题:** Slab/SLUB 对象级分配器
> **对标旧书:** ULK3 Ch8 / LKD3 Ch12

---

## 讲义要点

### 为什么需要 Slab/SLUB

伙伴系统只能按页分配（最小 4KB），但内核大量需要小对象（task_struct ~8KB, inode ~500B, sk_buff ~256B）。Slab/SLUB 在页之上提供对象池：

```
伙伴系统 → 分配 4KB 页 → SLUB 切分为 N 个对象 → 对象级分配
  kmalloc(256) → 从 kmalloc-256 cache 取一个对象 (~20ns, 无锁)
```

### SLUB 结构

```c
// per-CPU 快路径
struct kmem_cache_cpu {
    void **freelist;        // 当前 slab 的空闲对象链表
    struct page *page;      // 当前 slab 页
    struct page *partial;   // partial slab 链表
};

// per-node 慢路径
struct kmem_cache_node {
    spinlock_t list_lock;
    unsigned long nr_partial;
    struct list_head partial;  // partial slab 链表
};
```

### 分配路径

```
kmalloc(256):
  1. 找到 kmalloc-256 cache
  2. 快路径: cpu_slab->freelist 有空闲 → 取一个, 移动 freelist (~20ns)
  3. 中路径: freelist 空, cpu_slab->partial 有 slab → 切换到 partial slab
  4. 慢路径: partial 空, 从 node->partial 取 slab → 补充 cpu_slab
  5. 最慢路径: 都空, 从伙伴系统分配新页 → 切分为对象
```

### kmalloc cache 大小

```bash
# 查看 kmalloc cache 大小
ls /sys/kernel/slab/ | grep kmalloc
# kmalloc-8  kmalloc-16  kmalloc-32  kmalloc-64
# kmalloc-96  kmalloc-128  kmalloc-192  kmalloc-256
# kmalloc-512  kmalloc-1k  kmalloc-2k  kmalloc-4k
# kmalloc-8k  ...

# 每个大小一个 cache, SLUB 会合并相近大小
```

---

## 动手实验

```bash
# 1. 查看 SLUB 统计
cat /proc/slabinfo | head -20

# 2. slabtop 实时查看
slabtop -o | head -30

# 3. 查看特定 cache 信息
cat /sys/kernel/slab/kmalloc-256/objs_per_slab
cat /sys/kernel/slab/kmalloc-256/order
cat /sys/kernel/slab/kmalloc-256/slabs

# 4. 启用 SLUB 调试
# 内核启动参数: slub_debug=FZP (F=redzone, Z=poison, P=tracking)
echo 1 > /sys/kernel/slab/kmalloc-256/sysfs_check
```

---

## 与旧书差异

| ULK3 | Bootlin 讲义 |
|------|-------------|
| SLAB 为主 | SLUB 为默认 (SLAB 已移除) |
| array_cache | per-CPU freelist |
| 无 sysfs 接口 | `/sys/kernel/slab/` 丰富调试 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** kmalloc(300) 实际从哪个 cache 分配？对象大小是多少？

> 从 `kmalloc-512` 分配（向上对齐到最近的 cache 大小）。实际对象大小 512 字节（含 SLUB metadata）。SLUB 的 cache 大小是 2 的幂次序列（8,16,32,...,512）加几个特殊大小（96,192）。300 字节向上取 512，浪费 212 字节。

**Q2:** SLUB 的 partial slab 链表有什么作用？

> 当一个 slab 页的所有对象都被分配完时，它从 per-CPU freelist 移除但保留在 partial 链表中。当对象被释放时，该 slab 重新有空闲对象。如果 partial 中有 slab，释放的对象直接放回该 slab。partial 链表避免频繁向伙伴系统申请新页，也避免完全空闲的 slab 被立即释放（因为可能马上又需要）。

</details>
