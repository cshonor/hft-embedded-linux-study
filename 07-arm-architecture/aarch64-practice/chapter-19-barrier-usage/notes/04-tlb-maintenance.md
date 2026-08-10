# §19.4 案例四：TLB 维护

> **来源：** [Ch19 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

修改页表后 TLB 维护的屏障序列：TLBI（刷 TLB）→ DSB ISH（等刷新完成）→ ISB（重新取指）。本节分析为什么需要 DSB+ISB、各指令的必要性，以及多核 TLB shootdown 的屏障开销。

## 核心要点

### TLB 维护屏障序列

```c
// 修改页表项后
set_pte(ptep, new_pte);
// 必须：先失效 TLB
tlbi vae1is, vaddr;   // Inner Shareable，刷指定 VA
dsb ish;               // 等 TLB 刷新完成
isb;                    // 重新取指
```

### 完整序列代码

```asm
// TLB 维护完整序列
str x1, [x0]          // 1. 修改页表项（PTE）
tlbi vae1is, x2       // 2. 刷 TLB（广播到所有核）
dsb ish               // 3. 等 TLB 刷新完成
isb                   // 4. 冲刷流水线

// 现在可以安全释放旧物理页
```

### 为什么用 DSB ISH + ISB？

| 指令 | 作用 | 不做后果 |
|------|------|---------|
| `tlbi vae1is` | 刷指定 VA 的 TLB（Inner Shareable，所有核） | CPU 用旧 TLB 条目翻译 → 访问错误 PA |
| `dsb ish` | 等 TLB 刷新在所有 Inner Shareable CPU 上**完成** | TLB 刷新未完成就继续 → 仍用旧 TLB |
| `isb` | 本核流水线中可能有旧指令使用旧 TLB → 必须**冲刷** | 流水线中有旧 TLB 翻译的指令 |

> 三者缺一不可，顺序不能变。

### 各指令的必要性

| 指令 | 不做会怎样 | 为什么 |
|------|-----------|--------|
| `tlbi` | CPU 用旧 TLB → 访问错误 PA | TLB 是页表的 cache，修改后需失效 |
| `dsb ish` | TLB 刷新未完成就继续 | TLB 刷新异步，需等待 |
| `isb` | 流水线中有旧翻译指令 | 流水线中可能预取了用旧 TLB 的指令 |

### 为什么用 ish 不用 sy？

| 作用域 | 适用 | 原因 |
|--------|------|------|
| `ish` | TLB 维护 | TLB 是 CPU 内部的，只 CPU 核间共享 |
| `sy` | DMA | 涉及外部设备，需全系统 |

`ish` 比 `sy` 轻量——TLB 与 DMA 无关，不需要全系统作用域。

### 多核 TLB shootdown 开销

| 操作 | 延迟 | 说明 |
|------|------|------|
| `tlbi vae1is`（单页） | ~20-30ns | 广播到所有核 |
| `tlbi alle1is`（全刷） | ~50-100ns | 广播 + 所有核全刷 |
| `dsb ish` | ~30-50ns | 等所有核完成 |
| `isb` | ~10-20ns | 流水线冲刷 |
| 总计（单页） | ~60-100ns | vae1is + dsb + isb |
| 总计（全刷） | ~90-170ns | alle1is + dsb + isb |
| + TLB rebuild | ~200-400ns/miss | 后续访存 TLB miss |

## HFT 关联

TLB 维护在 HFT 系统中应尽量避免——`tlbi` + `dsb` + `isb` 序列的延迟约 50-100ns（TLB 刷新 + 等待 + 流水线冲刷），加上后续 TLB miss 重建的开销。

### HFT 避免 TLB 维护策略

```c
// HFT 最佳实践：静态映射，运行时不修改页表
void hft_init_memory(void) {
    // 启动时建立所有映射
    map_all_code();        // 代码段
    map_all_data();       // 数据段
    map_hot_data_2mb();   // 热数据用大页
    warmup_tlb();         // 预热 TLB
    // 之后零 TLB 维护
}

// 如果必须修改页表（管理核）
void safe_page_update(pte_t *pte, unsigned long va, pte_t new) {
    *pte = 0;                               // break
    asm volatile("dc cvac, %0" :: "r"(pte));
    asm volatile("tlbi vae1is, %0" :: "r"(va));  // 只刷一个 VA
    asm volatile("dsb ish" ::: "memory");
    *pte = new;                             // make
    asm volatile("dc cvac, %0" :: "r"(pte));
    asm volatile("tlbi vae1is, %0" :: "r"(va));
    asm volatile("dsb ish" ::: "memory");
    asm volatile("isb" ::: "memory");
}
```

HFT 系统应静态映射所有内存（启动时建页表，运行时不修改）。如果必须修改页表，用 `vae1is`（按 VA 精确刷）而非 `alle1is`（全刷），减少 TLB rebuild 范围。在多核 HFT 中，`tlbi is`（Inner Shareable）会触发其他核的 TLB shootdown，延迟更大——确保只在必要时修改共享页表。

## 自测题

1. **TLB 维护中 `dsb ish` 的作用是什么？为什么用 ish 而不是 sy？**

<details>
<summary>答案</summary>

`dsb ish` 等 TLB 刷新在所有 **Inner Shareable** CPU 上完成。用 `ish` 而不是 `sy` 是因为 TLB 是 CPU 内部的，只在 CPU 核间共享（Inner Shareable），不需要扩展到全系统（`sy` 包括 DMA 等外部设备，TLB 与 DMA 无关）。`ish` 比 `sy` 轻量。
</details>

2. **TLB 维护中 ISB 的作用是什么？不跟 ISB 会怎样？**

<details>
<summary>答案</summary>

ISB 冲刷**流水线**，确保后续指令用新 TLB 重新翻译取指。不跟 ISB：流水线中可能有在 TLB 刷新前取的指令，这些指令使用旧 TLB 映射 → 执行后访问错误 PA。即使 `dsb` 等 TLB 刷新完成，流水线中的旧指令仍可能用旧映射——只有 ISB 能冲刷流水线。
</details>

3. **修改页表后只用 `tlbi` 不用 `dsb` 和 `isb`，会怎样？**

<details>
<summary>答案</summary>

1. 不用 `dsb`：TLB 刷新是异步的，`tlbi` 发出后不立即完成。没有 `dsb` 等待 → 后续指令可能在 TLB 还没刷新完时就执行 → 仍用旧 TLB 条目。
2. 不用 `isb`：流水线中有旧 TLB 翻译的指令 → 行为不可预测。

正确序列 `tlbi` → `dsb` → `isb` 三者缺一不可。
</details>

4. **HFT 系统中如何避免 TLB shootdown 的开销？**

<details>
<summary>答案</summary>

1. **静态映射**：启动时建立所有页表，运行时不修改 → 零 TLB 维护
2. **大页**：用 2MB/1GB 大页减少 TLB 条目数 → 减少 shootdown 影响
3. **绑核**：线程绑定 CPU 避免进程切换 → 不触发 TLB flush
4. **避免 mprotect/munmap**：运行时不修改页表
5. **精确刷**：必须修改时用 `vae1is`（单页）而非 `alle1is`（全刷）
</details>

## 参考与延伸

- [§19.5 屏障选择决策树](05-decision-tree.md) — TLB 在决策树中的位置
- [Ch17 §17.3 TLB 刷新指令](../../chapter-17-tlb-management/notes/section-0-本章完整概述.md) — TLBI 指令详解
- [Ch18 §18.2 三条屏障指令](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md) — DSB/ISB 详解
