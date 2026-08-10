# memblock (取代 bootmem)

> **原文:** [memblock: The early boot memory allocator](https://lwn.net/Articles/449283/) (LWN, 2011)
> **内核版本:** 3.9+ (bootmem 被移除)
> **对标旧书:** ULK3 Appendix E (bootmem 分配器)

---

## 核心观点

memblock 取代了旧的 bootmem 分配器，成为内核启动早期的内存管理器。

### bootmem 的问题

bootmem 使用位图管理物理内存，每个物理页对应 1 bit：
- 大内存系统位图很大（128GB = 4MB 位图）
- 位图本身需要分配内存（鸡生蛋问题）
- 只支持 first-fit 分配，碎片严重

### memblock 设计

```c
// 源码路径: include/linux/memblock.h
struct memblock {
    bool bottom_up;              // 分配方向
    bool current_limit;
    struct memblock_type memory;  // 可用内存区域
    struct memblock_type reserved; // 已保留区域
};

struct memblock_type {
    unsigned long cnt;            // 区域数量
    unsigned long max;            // 最大区域数
    struct memblock_region *regions;
};

struct memblock_region {
    phys_addr_t base;            // 起始物理地址
    phys_addr_t size;            // 大小
    int nid;                     // NUMA 节点
    unsigned long flags;         // MEMBLOCK_NOMAP 等
};
```

### memblock 分配流程

```c
// 早期启动阶段 (buddy 系统未初始化)
// 源码路径: mm/memblock.c

// 1. 从固件 (DT/ACPI) 获取可用内存区域
memblock_add(base, size);      // 添加可用内存
memblock_reserve(base, size);   // 保留区域 (内核镜像等)

// 2. 早期分配
phys_addr_t memblock_phys_alloc(size, align);
void *memblock_alloc(size, align);  // 返回虚拟地址

// 3. buddy 系统初始化后，memblock 内存移交给 buddy
// memblock_free_all() → __free_pages() 逐页释放给 buddy
```

### ARM64 启动序列

```c
// arch/arm64/kernel/setup.c
void __init arm64_memblock_init(void)
{
    // 1. 从 DT 获取内存布局
    memblock_enforce_memory_limit();
    
    // 2. 保留内核镜像
    memblock_reserve(__pa(_text), _end - _text);
    
    // 3. 保留 initrd
    memblock_reserve(initrd_start, initrd_end - initrd_start);
    
    // 4. 保留 DTB
    memblock_reserve(__pa(initial_boot_params), fdt_totalsize(...));
}
```

---

## 与旧书差异

| ULK3 讲的 | 现代实现 |
|-----------|---------|
| bootmem 位图分配器 | memblock 区域数组 (3.9+) |
| `alloc_bootmem()` | `memblock_alloc()` |
| `free_bootmem()` | `memblock_free()` |
| `bootmem_data_t` | `struct memblock` |

---

## HFT 关联

memblock 在启动早期使用，HFT 系统启动后不再涉及。但理解 memblock 有助于调试启动阶段内存布局问题（如大页预留、initrd 位置）。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** memblock 为什么比 bootmem 更适合大内存系统？

> bootmem 用位图，128GB 内存需要 4MB 位图，且位图本身需要 bootmem 分配（鸡生蛋）。memblock 用区域数组，128GB 内存可能只有几个区域条目，元数据极小。memblock 还支持 bottom-up 分配（从高地址向低地址分配），避免与内核镜像冲突。

**Q2:** memblock 什么时候把内存移交给 buddy 系统？

> 在 `mm_init()` → `memblock_free_all()` 中，memblock 遍历所有可用内存区域，将每个页通过 `__free_pages()` 释放给 buddy 系统。之后 memblock 不再用于分配，但保留的 `reserved` 区域信息仍然有效（用于查询保留区域）。

</details>
