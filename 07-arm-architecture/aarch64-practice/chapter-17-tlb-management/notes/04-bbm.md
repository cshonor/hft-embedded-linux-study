# §17.4 BBM（Break-Before-Make）

> **来源：** [Ch17 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

修改页表项（从有效→有效，改映射）时必须遵循 BBM 协议：先失效旧映射（break）→ TLB 刷新 → DSB → 写新映射（make）。本节分析 BBM 的原理、完整步骤、适用场景和在内核中的实现。

## 核心要点

### BBM 步骤

```
1. 将页表项设为 Invalid（break）
   → str xzr, [pte_addr]   // 写 0（Invalid descriptor）

2. TLB 刷新（确保旧映射失效）
   → tlbi vae1is, x0       // x0 = 被修改的 VA

3. DSB（等待 TLB 刷新在所有核完成）
   → dsb sy                // 必须等待，不能跳过

4. 写入新的有效映射（make）
   → str x1, [pte_addr]    // x1 = 新 PTE 值

5. TLB 刷新（可选，确保新映射可见）
   → tlbi vae1is, x0       // 可选

6. DSB + ISB
   → dsb sy
   → isb
```

### BBM 汇编实现

```asm
// BBM: 修改 VA 的映射从 PA_old 到 PA_new
// x0 = PTE 地址, x1 = 新 PTE 值, x2 = VA

break:
    str     xzr, [x0]          // Step 1: 设为 Invalid（break）
    
    tlbi    vae1is, x2         // Step 2: 刷 TLB（广播到所有核）
    dsb     sy                 // Step 3: 等待刷新完成

make:
    str     x1, [x0]          // Step 4: 写新映射（make）
    
    tlbi    vae1is, x2         // Step 5: 刷新（可选）
    dsb     sy                 // Step 6: 等待
    isb                        // 确保后续指令用新映射
```

### 为什么需要 BBM？

| 不遵循 BBM | 后果 |
|-----------|------|
| 直接改页表项（旧→新） | 在 TLB 刷新之前，其他核可能用旧 TLB → 访问旧 PA |
| 旧 PA 已被释放/重用 | 数据损坏或信息泄漏 |
| 新旧映射同时可见 | 两个核看到不同映射 → 数据不一致 |

### BBM 时序对比

```
不遵循 BBM（危险）：
T0: 核A 写新 PTE（直接覆盖）
T1: 核B 用旧 TLB 访问 VA → 到达旧 PA  ← 危险！
T2: 核A 释放旧 PA
T3: 核B 仍可能访问旧 PA（use-after-free）

遵循 BBM（安全）：
T0: 核A 设 PTE=Invalid（break）
T1: 核A TLBI vae1is（广播）
T2: 核B 用 TLB → Invalid → 触发 Page Fault（暂停）
T3: 核A DSB（等所有核 TLB 刷新完成）
T4: 核A 写新 PTE（make）
T5: 核B 重试 → TLB miss → walk 新页表 → 到达新 PA ✓
```

> 不遵循 BBM → 在 break 和 make 之间，其他核可能用旧 TLB → 访问已释放的物理页 → 数据损坏。
> 内核的 `set_pte()` 通常封装了 BBM 逻辑。

### BBM 适用场景

| 场景 | 需要 BBM？ | 原因 |
|------|-----------|------|
| Invalid → Valid（新映射） | 不需要 | 没有旧映射需要 break |
| Valid → Valid（改映射） | **需要** | 旧映射可能还在 TLB 中 |
| Valid → Invalid（取消映射） | 不需要 | 只 break 不 make |
| 修改 PTE 权限位 | **需要** | 旧权限可能还在 TLB 中 |
| 修改 PTE 物理地址 | **需要** | 旧 PA 可能还在 TLB 中 |

### Linux 内核中的 BBM

```c
// Linux set_pte_at 封装了 BBM 逻辑（简化）
void set_pte_at(struct mm_struct *mm, unsigned long addr,
                pte_t *ptep, pte_t pte) {
    pte_t old_pte = READ_ONCE(*ptep);
    
    if (pte_valid(old_pte) && pte_valid(pte)) {
        // Valid → Valid：需要 BBM
        // Step 1: break
        set_pte(ptep, __pte(0));  // 设 Invalid
        // Step 2: flush TLB
        flush_tlb_page(vma, addr);
        // Step 3: DSB（在 flush_tlb_page 内部）
        // Step 4: make
        set_pte(ptep, pte);
    } else {
        // Invalid → Valid 或 Valid → Invalid：直接写
        set_pte(ptep, pte);
        if (pte_valid(pte))
            flush_tlb_page(vma, addr);
    }
}
```

### BBM 与 COW（Copy-On-Write）

```
COW 流程中的 BBM：
1. fork 时：父进程 PTE 改为只读（Valid→Valid，需 BBM）
   break: PTE=Invalid → TLBI → DSB
   make:  PTE=只读（原 PA）
   
2. 子进程建立映射（Invalid→Valid，不需要 BBM）
   直接 make: PTE=只读（同 PA）

3. 写时触发 Page Fault：
   break: PTE=Invalid → TLBI → DSB
   分配新物理页，复制数据
   make:  PTE=可写（新 PA）
```

## HFT 关联

BBM 在 HFT 多核系统中很重要——如果交易核和管理核共享页表，管理核修改映射时必须遵循 BBM，否则交易核可能用旧 TLB 访问已释放的物理页。

在裸金属 HFT 中，通常不动态修改页表（启动时建好映射后不再变），BBM 不是日常问题。但如果需要动态映射内存（如运行时分配大页），必须遵循 BBM。Linux 的 `set_pte_at()` 已封装 BBM，不需要手动处理。

### HFT 静态映射建议

```c
// HFT 最佳实践：启动时建立所有映射，运行时不修改
void hft_init_memory(void) {
    // 1. 映射所有代码段（只读+可执行）
    map_range(code_start, code_size, PTE_RX);
    
    // 2. 映射所有数据段（读写）
    map_range(data_start, data_size, PTE_RW);
    
    // 3. 映射所有热数据（大页 2MB）
    map_range_2mb(hot_data, hot_size, PTE_RW | PTE_HUGE);
    
    // 4. 预热 TLB
    warmup_tlb();
    
    // 之后不再修改页表 → 不需要 BBM
}
```

## 自测题

1. **BBM 是什么？不遵循会有什么后果？**

<details>
<summary>答案</summary>

BBM = **Break-Before-Make**：修改页表映射时，先失效旧映射（break），TLB 刷新后再写新映射（make）。不遵循的后果：直接改页表项后，在 TLB 刷新前，其他核可能仍用旧 TLB 条目 → 访问**旧 PA**（可能已释放/重用）→ **数据损坏**。
</details>

2. **什么情况下不需要 BBM？**

<details>
<summary>答案</summary>

- **Invalid → Valid**（新映射）：没有旧映射需要 break，直接 make
- **Valid → Invalid**（取消映射）：只有 break 没有 make

只有 **Valid → Valid**（改映射）才需要 BBM——先 break 旧映射，再 make 新映射。
</details>

3. **BBM 步骤中为什么 break 后要 DSB 才能 make？**

<details>
<summary>答案</summary>

break（设 Invalid）+ TLB 刷新后，TLB 刷新是**异步**的——其他核可能还没完成刷新。DSB 等待所有核的 TLB 刷新完成，确保没有核还在用旧映射。在 DSB 完成前写新映射（make），其他核可能同时看到新旧映射 → 数据不一致。DSB 是 break 和 make 之间的安全屏障。
</details>

4. **COW（Copy-On-Write）流程中哪些步骤需要 BBM？**

<details>
<summary>答案</summary>

需要 BBM 的步骤：
1. **fork 时父进程 PTE 改为只读**（Valid→Valid）：需要 BBM，先 break 再设只读
2. **写时 Page Fault 处理**：PTE 从旧 PA 只读改为新 PA 可写（Valid→Valid），需要 BBM

不需要 BBM 的步骤：
- 子进程建立新映射（Invalid→Valid）：直接 make
- 取消映射（Valid→Invalid）：只 break 不 make
</details>

## 参考与延伸

- [§17.3 TLB 刷新指令](03-tlb-flush.md) — BBM 中使用的 TLBI 指令
- [§17.7 易错点](07-pitfalls.md) — BBM 相关错误
- [Ch18 §18.2 三条屏障指令](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md) — DSB 在 BBM 中的作用
