# Bootlin: 页表与 TLB 管理

> **来源:** [Bootlin Kernel Training — Memory Management](https://bootlin.com/docs/kernel/)
> **主题:** 多级页表、TLB 管理、huge page
> **对标旧书:** ULK3 Ch3 / LKD3 Ch3

---

## 讲义要点

### 页表层级

```
ARM64 (4KB 页, 4 级):
  虚拟地址 [63:48] 保留 [47:39] PGD [38:30] PUD [29:21] PMD [20:12] PTE [11:0] offset
                                    512     512     512     512
  每级 9 bit 索引, 512 个条目 (9b × 4 = 36b + 12b offset = 48b)

ARM64 (64KB 页, 3 级):
  虚拟地址 [63:42] 保留 [41:29] PGD [28:16] PUD [15:3] PTE [2:0] offset
  每级 13 bit 索引, 8192 个条目 (页表页 = 64KB)
```

### 页表项 (PTE) 格式

```
ARM64 PTE (4KB 页):
  bit 0:    valid (0=invalid, 1=valid)
  bit 1:    large page (PMD level)
  bit 6:    accessed (young)
  bit 7:    dirty
  bit 51-12: physical address [51:12] (40 bit, 1TB 物理地址)
  bit 53:   DBM (dirty bit modifier)
  bit 54:   contiguous
  bit 55:   PXN (privileged execute-never)
  bit 56:   UXN (user execute-never)
  bit 58-61: software bits
  bit 10-11: AP (access permission)
```

### TLB 管理

```c
// ARM64 TLB 指令
// TLB IPAS2E1IS, Xn  — 无效化 IPA (Stage 2, inner shareable)
// TLB VAE1IS, Xn      — 无效化虚拟地址 (Stage 1, inner shareable)
// TLB ASIDE1IS, Xn    — 无效化 ASID (inner shareable)
// TLB ALLE1IS         — 无效化所有 (Stage 1, inner shareable)

// 内核 API:
// 源码路径: arch/arm64/include/asm/tlbflush.h
void flush_tlb_mm(struct mm_struct *mm);
void flush_tlb_range(struct vm_area_struct *vma, unsigned long start, unsigned long end);
void flush_tlb_page(struct vm_area_struct *vma, unsigned long addr);
```

### 大页 (Huge Pages)

```bash
# 2MB 大页 (ARM64 4KB 页, order 9)
echo 1024 > /proc/sys/vm/nr_hugepages  # 预留 1024 个 2MB 大页 = 2GB

# 透明大页 (THP)
cat /sys/kernel/mm/transparent_hugepage/enabled
# [always] madvise never

# 1GB 大页 (ARM64 4KB 页, order 18)
# 需要 CPU 支持和内核配置 CONFIG_HUGETLBFS
```

### TLB shootdown

```
CPU 0 修改页表 (无效化 PTE)
  → CPU 0 本地 TLB flush (单核操作)
  → 发送 IPI 给 CPU 1,2,3 (TLB shootdown)
  → CPU 1,2,3 收到 IPI, flush 本地 TLB
  → CPU 0 等待所有 CPU 确认 (barrier)

开销: 每次 shootdown ~1-5μs (IPI + 等待 + barrier)
```

---

## 动手实验

```bash
# 1. 查看 TLB 信息
cat /proc/cpuinfo | grep -i tlb

# 2. 查看大页信息
cat /proc/meminfo | grep -i huge

# 3. 使用大页
# 方法 A: mmap(MAP_HUGETLB)
# 方法 B: mount hugetlbfs
mount -t hugetlbfs none /mnt/huge
# 方法 C: 透明大页
echo always > /sys/kernel/mm/transparent_hugepage/enabled

# 4. 查看 THP 状态
cat /sys/kernel/mm/transparent_hugepage/defrag
```

---

## 与旧书差异

| ULK3 | Bootlin 讲义 |
|------|-------------|
| 3 级页表 (32-bit) | 4 级 (48-bit) / 5 级 (57-bit) |
| 无 ASID | ARM64 ASID 减少 TLB flush |
| 无 THP | 透明大页 (2.6.38+) |
| TLB flush 全量 | 精细化 flush (by ASID/VA) |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** ARM64 的 ASID 如何减少 TLB flush？

> ASID (Address Space ID) 是 8/16 位标识符，每个进程一个 ASID。TLB 条目包含 ASID，查找时只匹配当前 ASID 的条目。进程切换时不需要 flush TLB——新进程的虚拟地址映射不同 ASID，不会冲突。只有 ASID 耗尽（16 位 = 65536 个）时才需要全局 flush。这比 x86 的 PCID 更高效（x86 PCID 也有类似机制）。

**Q2:** TLB shootdown 对 HFT 有什么影响？如何避免？

> TLB shootdown 通过 IPI 让其他 CPU flush TLB，延迟 1-5μs。HFT 交易线程如果在 shootdown 期间运行，会被 IPI 中断打断。避免方法：(1) 减少 `mprotect`/`munmap` 操作（触发 shootdown）；(2) 用大页减少 PTE 数量；(3) 绑核隔离交易线程，减少其他进程的 shootdown IPI 到达交易核。

</details>
