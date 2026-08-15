# Bootlin: vmalloc 与非连续内存

> **来源:** [Bootlin Kernel Training — Memory Management](https://bootlin.com/docs/kernel/)
> **主题:** vmalloc 非连续物理内存分配
> **对标旧书:** ULK3 Appendix G / LKD3 Ch12

---

## 讲义要点

### vmalloc 原理

```c
// vmalloc 分配物理不连续但虚拟连续的内存
// 源码路径: mm/vmalloc.c

void *vmalloc(unsigned long size);
// 1. 在 VMALLOC_START ~ VMALLOC_END 范围找空闲虚拟地址区间
// 2. 逐页调用 alloc_page() 分配物理页 (物理不连续)
// 3. 修改页表映射虚拟→物理
// 4. 返回虚拟地址

// 与 kmalloc 对比:
// kmalloc: 物理连续 + 虚拟连续, 限制 ~4MB (伙伴系统 MAX_ORDER)
// vmalloc: 物理不连续 + 虚拟连续, 可达 GB 级
```

### 地址空间布局 (ARM64)

```
64-bit 虚拟地址空间 (48-bit):
  0x0000_0000_0000_0000 ~ 0x0000_7FFF_FFFF_FFFF  用户空间 (128TB)
    ├── text段
    ├── data/bss段
    ├── heap (brk)
    ├── mmap 区域 (向下增长)
    └── stack

  0xFFFF_8000_0000_0000 ~ 0xFFFF_FFFF_FFFF_FFFF  内核空间 (128TB)
    ├── 直接映射区 (PAGE_OFFSET ~ )  — 物理内存 1:1 映射
    ├── VMALLOC_START ~ VMALLOC_END   — vmalloc 区域
    ├── vmemmap_start ~              — struct page 数组 (SPARSEMEM)
    └── 固定映射区
```

### vmalloc 性能特征

| 特性 | kmalloc | vmalloc |
|------|---------|---------|
| 物理连续 | 是 | 否 |
| 分配大小限制 | ~4MB (MAX_ORDER) | GB 级 |
| 分配速度 | ~20ns (快路径) | ~μs (修改页表) |
| TLB | 友好 (物理连续) | 不友好 (每页独立 PTE) |
| DMA | 支持 | 不直接支持 |
| 适用场景 | 小对象, DMA | 大缓冲区, 模块 |

---

## 动手实验

```bash
# 1. 查看 vmalloc 区域
cat /proc/vmallocinfo | head -20
# 0xffff800000100000-0xffff800000101000 4096 ...
# 显示虚拟地址范围、大小、调用者

# 2. 查看 vmalloc 总用量
cat /proc/vmallocinfo | awk '{sum += $2} END {print sum/1024/1024 " MB"}'

# 3. 查看地址空间布局
cat /proc/<pid>/maps

# 4. 内核模块中用 vmalloc
# modprobe my_module  # 内部 vmalloc(1MB)
```

---

## 与旧书差异

| ULK3 | Bootlin 讲义 |
|------|-------------|
| `VMALLOC_START` 固定 | 现代内核动态计算 |
| 无 vmap_area 红黑树 | 5.x+ 用红黑树管理 vmalloc 区间 |
| `__get_vm_area()` | `__get_vm_area_caller()` |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么 vmalloc 比 kmalloc 慢？

> vmalloc 需要：(1) 查找空闲虚拟地址区间（红黑树搜索）；(2) 逐页 `alloc_page()` 分配物理页（多次伙伴系统调用）；(3) 逐页修改内核页表（`set_pte_at()`，每次 TLB flush）。kmalloc 快路径仅需一次 per-CPU freelist 指针移动。vmalloc 总体慢 100-1000 倍。

**Q2:** 什么场景应该用 vmalloc 而不是 kmalloc？

> (1) 需要大块内存（>4MB）且物理连续性不要求（如日志缓冲区）；(2) 内核模块加载时分配模块代码/数据区；(3) 大型查找表（如路由表哈希表）。不适用于 DMA（需要物理连续）和延迟敏感路径（页表修改开销）。

</details>
