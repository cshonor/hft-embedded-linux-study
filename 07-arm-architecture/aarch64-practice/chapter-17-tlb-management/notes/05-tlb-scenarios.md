# §17.5 内核 TLB 维护场景

> **来源：** [Ch17 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

Linux 内核中各种 TLB 维护场景：进程切换（有/无 ASID）、munmap、fork COW、kmap/kunmap、模块加载。不同场景用不同的 TLBI 指令。本节给出每种场景的 TLB 操作流程和指令选择。

## 核心要点

### 场景与操作总览

| 场景 | 操作 | TLBI 指令 | 需要广播？ |
|------|------|-----------|-----------|
| 进程切换（无 ASID） | `TLBI alle1` 全刷 | alle1is | 是 |
| 进程切换（有 ASID） | 换 ASID，不刷 | 不需要 | — |
| munmap | `TLBI vae1` 刷指定 VA | vae1is | 是 |
| fork → COW | 刷新被修改的页 | vae1is | 是 |
| kmap/kunmap | `TLBI vae1` 或 `TLBI alle1` | vae1is | 是 |
| 模块加载 | 无需（代码在内核空间） | 不需要 | — |
| mprotect | 刷被修改的页 | vae1is | 是 |
| 页迁移 | BBM 刷旧+新 | vae1is × 2 | 是 |

### 进程切换 TLB 策略

```c
// 有 ASID 的进程切换（现代 ARM）
void switch_mm(struct mm_struct *prev, struct mm_struct *next) {
    // 只换 ASID + TTBR0，不刷 TLB
    u64 ttbr = (next->asid << 48) | virt_to_phys(next->pgd);
    asm volatile("msr TTBR0_EL1, %0" :: "r"(ttbr));
    asm volatile("isb");
}

// 无 ASID 的进程切换（旧架构）
void switch_mm_no_asid(struct mm_struct *next) {
    u64 ttbr = virt_to_phys(next->pgd);
    asm volatile("msr TTBR0_EL1, %0" :: "r"(ttbr));
    asm volatile("tlbi alle1is");  // 全刷（所有核）
    asm volatile("dsb sy");
    asm volatile("isb");
}
```

| 有 ASID | 无 ASID |
|---------|---------|
| 换 TTBR0 的 ASID 字段 | `tlbi alle1is` 全刷 |
| 旧进程 TLB 保留 | 旧进程 TLB 全部失效 |
| 切回旧进程 TLB hot | 切回旧进程 TLB cold |
| 性能好 | 性能差 |

### munmap 完整流程

```c
// Linux 内核 munmap 的 TLB 维护（简化）
void munmap_range(struct mm_struct *mm, unsigned long start, 
                  size_t len) {
    // 1. 遍历页表，将 PTE 设为 Invalid
    for (each page in [start, start+len)) {
        ptep_get_and_clear(mm, addr, ptep);  // PTE = Invalid
    }
    
    // 2. 刷 TLB（广播到所有核）
    flush_tlb_range(vma, start, start + len);
    // 内部：tlbi vae1is 循环或 tlbi asides1is
    
    // 3. DSB 等待刷新完成
    dsb sy;
    
    // 4. 释放物理页
    free_pages(pfn, order);
}
```

```
munmap 时序：
1. 从页表删除映射（设 Invalid）
2. TLBI vae1is, vaddr  ← 刷指定 VA 的 TLB
3. DSB + ISB
4. 释放物理页
```

> 注意：必须先刷 TLB 再释放物理页，否则其他核可能用旧 TLB 访问已释放的页。

### fork → COW TLB 操作

```
fork COW 流程：

1. 父进程 PTE 改为只读（Valid→Valid，需 BBM）
   break: PTE=Invalid → tlbi vae1is → dsb
   make:  PTE=只读（原 PA，加 COW 标记）

2. 子进程建立映射（Invalid→Valid，不需要 BBM）
   直接 make: PTE=只读（同 PA）

3. 写时触发 Page Fault：
   break: PTE=Invalid → tlbi vae1is → dsb
   分配新物理页，复制数据
   make:  PTE=可写（新 PA）
```

### kmap/kunmap（临时映射）

```c
// kmap：建立临时映射
void *kmap(struct page *page) {
    unsigned long vaddr = get_kmap_addr();
    set_pte(kmap_pte, mk_pte(page, PAGE_KERNEL));
    // Invalid → Valid：不需要 BBM
    flush_tlb_kernel_range(vaddr, vaddr + PAGE_SIZE);
    // tlbi vae1is, vaddr
    return (void *)vaddr;
}

// kunmap：取消临时映射
void kunmap(void *vaddr) {
    pte_clear(kmap_pte);  // PTE = Invalid
    // Valid → Invalid：不需要 BBM
    flush_tlb_kernel_range(vaddr, vaddr + PAGE_SIZE);
    // tlbi vae1is, vaddr
}
```

### mprotect TLB 操作

```c
// mprotect：修改页面权限
// 例如：从 RW 改为 RO
void mprotect_range(struct mm_struct *mm, unsigned long start,
                    size_t len, pgprot_t new_prot) {
    for (each page in [start, start+len)) {
        pte_t old = ptep_get_and_clear(ptep);  // BBM: break
        pte_t new = pte_modify(old, new_prot);  // 改权限
        set_pte(ptep, new);                     // BBM: make
    }
    // Valid → Valid（改权限）：需要 BBM
    flush_tlb_range(vma, start, start + len);
}
```

### Linux TLB API 层次

| API | 底层 TLBI | 用途 |
|-----|---------|------|
| `local_flush_tlb_all()` | `tlbi alle1` | 刷本核全部 |
| `flush_tlb_all()` | `tlbi alle1is` | 刷所有核全部 |
| `flush_tlb_mm(mm)` | `tlbi aside1is` | 刷该进程（所有 VA） |
| `flush_tlb_page(vma, addr)` | `tlbi vae1is` | 刷单个 VA |
| `flush_tlb_range(vma, start, end)` | `tlbi vae1is` 循环 | 刷一段 VA 范围 |

## HFT 关联

HFT 系统在 Linux 上运行时，`munmap` 是 TLB 抖动的来源——释放内存时触发 `vae1is` TLB 刷新。HFT 应避免在交易路径上分配/释放内存（使用内存池）。

### HFT 避免 TLB 抖动策略

```c
// 1. 使用内存池，避免运行时 munmap
struct order_pool {
    char buf[ORDER_POOL_SIZE] __attribute__((aligned(2097152)));  // 2MB 对齐
    size_t offset;
};
// 启动时 mmap 一次，运行时从 pool 分配，不调用 munmap

// 2. 线程绑定 CPU 避免进程切换
cpu_set_t cpuset;
CPU_ZERO(&cpuset);
CPU_SET(trading_cpu, &cpuset);
pthread_setaffinity_np(thread, sizeof(cpuset), &cpuset);
// 避免被调度器切换 → 不触发 TLB flush

// 3. 大页减少 TLB 压力
madvise(buf, size, MADV_HUGEPAGE);
// 或 mmap(MAP_HUGETLB)
```

进程切换（`schedule`）如果无 ASID 会全刷 TLB，HFT 应将交易线程绑定到专用 CPU 核（`CPU affinity`），避免被调度器切换。kmap/kunmap 在 HFT 中应避免（使用大页 permanent mapping 替代临时映射）。

## 自测题

1. **有 ASID 和无 ASID 的进程切换，TLB 操作有什么不同？**

<details>
<summary>答案</summary>

- **无 ASID**：进程切换时 `tlbi alle1is` **全刷 TLB**，旧进程的 TLB 条目全部失效。切回旧进程时 TLB cold（全 miss），性能差。
- **有 ASID**：进程切换只**换 ASID**（写 TTBR0 高位），不刷 TLB。旧进程的 TLB 条目带旧 ASID 标签仍保留。切回旧进程时 TLB hot，性能好。
</details>

2. **munmap 时为什么要先刷 TLB 再释放物理页？**

<details>
<summary>答案</summary>

多核系统中，其他核可能还有该 VA 的 TLB 条目。如果先释放物理页再刷 TLB：其他核可能用旧 TLB 访问已释放的物理页 → **use-after-free**（数据损坏或信息泄漏）。必须先 `tlbi vae1is` 刷 TLB + DSB 等完成，确保所有核都不再用旧映射，然后才释放物理页。这就是 BBM 的思想。
</details>

3. **模块加载为什么不需要刷 TLB？**

<details>
<summary>答案</summary>

模块加载到**内核空间**（高地址 TTBR1），内核页表在所有进程间共享，TLB 条目在所有进程间有效。模块代码所在的虚拟地址在 TLB 中已有映射（内核空间通常用大块映射），不需要新增页表项。但如果模块加载涉及新分配内核页（如 vmalloc 区域），则需要刷 TLB。
</details>

4. **`flush_tlb_range` 内部用什么 TLBI 指令？为什么不直接用 `alle1is`？**

<details>
<summary>答案</summary>

`flush_tlb_range` 内部用 `tlbi vae1is` 循环刷新指定范围内的每个 VA。不用 `alle1is` 是因为：
- `alle1is` 刷全部 TLB（所有进程），后续大量 TLB miss
- `vae1is` 只刷被修改的 VA，其他 TLB 条目保留
- 精确刷新影响范围小，性能好

但如果修改的范围很大（如几百个页），`vae1is` 循环的开销可能超过一次 `alle1is`，内核会自动选择更优方案。
</details>

## 参考与延伸

- [§17.2 ASID](02-asid.md) — ASID 机制详解
- [§17.3 TLB 刷新指令](03-tlb-flush.md) — 各种 TLBI 指令
- [§17.4 BBM](04-bbm.md) — munmap 中的 BBM
