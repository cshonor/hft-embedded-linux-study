# §17.7 易错点清单

> **来源：** [Ch17 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

TLB 管理的 6 个常见错误：改页表不刷 TLB、TLBI 后不跟 DSB/ISB、不用 ASID 全刷 TLB、不遵循 BBM、单核刷 TLB 忘了 `is` 后缀、修改页表后先释放物理页再刷 TLB。

## 核心要点

| # | 易错点 | 后果 | 修复 |
|---|--------|------|------|
| 1 | 改页表不刷 TLB | CPU 用旧映射，访问错误物理页 | 改页表后必须 TLBI |
| 2 | TLBI 后不跟 DSB/ISB | 刷新未完成就继续执行，行为未定义 | TLBI → DSB → ISB |
| 3 | 不用 ASID 全刷 TLB | 进程切换性能差（TLB cold miss 暴增） | 启用 ASID，按 ASID 刷 |
| 4 | 不遵循 BBM | 多核下可能同时看到新旧映射 → 数据损坏 | break → flush → DSB → make |
| 5 | 多核忘加 `is` 后缀 | 只刷了本核，其他核仍用旧 TLB | 多核场景用 `is` 变体 |
| 6 | 先释放物理页再刷 TLB | 其他核用旧 TLB 访问已释放页 → UAF | 先 TLBI+DSB 再释放 |

### 常见代码错误对比

| # | 错误代码 | 正确代码 | 问题 |
|---|---------|---------|------|
| 1 | `str new_pte, [pte]` 然后释放物理页 | `str xzr, [pte]; tlbi vae1is; dsb; 释放物理页` | 忘了刷 TLB 就释放 |
| 2 | `tlbi vae1; ...` （无 DSB） | `tlbi vae1; dsb sy; isb` | 缺 DSB+ISB |
| 3 | 多核：`tlbi vae1` | 多核：`tlbi vae1is` | 缺 `is`，其他核没刷 |
| 4 | 直接覆盖 PTE 值 | BBM: 先 Invalid → TLBI → DSB → 写新值 | 不遵循 BBM |
| 5 | `set_pte(ptep, new); free_page(pfn);` | `set_pte(ptep, 0); tlbi; dsb; free_page(pfn);` | 先释放后刷 |

### TLBI 指令选择速查

| 场景 | 正确指令 | 常见错误 |
|------|---------|---------|
| 刷单个页（多核） | `tlbi vae1is` | 用 `vae1`（只刷本核） |
| 刷整个进程（多核） | `tlbi aside1is` | 用 `alle1is`（刷了所有进程） |
| 刷所有（多核） | `tlbi alle1is` | 用 `alle1`（只刷本核） |
| 刷单个页（单核） | `tlbi vae1` | 用 `vae1is`（多余，但无害） |
| 进程退出 | `tlbi aside1is` | 用 `alle1is`（影响太大） |

### TLBI 序列完整性检查

```asm
// ✓ 完整的正确序列
str x0, [pte_addr]          // 修改页表
tlbi vae1is, x1             // 刷 TLB（广播）
dsb sy                       // 等待完成
isb                          // 冲刷流水线

// ✗ 错误1：缺 DSB
str x0, [pte_addr]
tlbi vae1is, x1
isb                          // TLB 刷新可能没完成！

// ✗ 错误2：缺 ISB
str x0, [pte_addr]
tlbi vae1is, x1
dsb sy                       // 流水线中有旧翻译！

// ✗ 错误3：缺 is（多核）
str x0, [pte_addr]
tlbi vae1, x1                // 只刷了本核！
dsb sy
isb
```

### 调试技巧

| 症状 | 可能原因 | 检查方向 |
|------|----------|---------|
| 修改页表后访问错误地址 | 没刷 TLB（用旧映射） | 在页表修改后加 TLBI |
| TLBI 后行为异常 | 没跟 DSB/ISB | 检查 TLBI 后是否有 DSB+ISB |
| 进程切换后性能暴跌 | 无 ASID，全刷 TLB | 检查 TCR_EL1.AS 是否启用 |
| 多核数据损坏 | 没遵循 BBM | 检查页表修改是否 BBM |
| 多核行为不一致 | 缺 `is` 后缀 | 多核场景用 `is` 变体 |
| Use-after-free | 先释放页后刷 TLB | 先刷 TLB + DSB 再释放 |

## HFT 关联

TLB 错误在 HFT 系统中通常导致不可预测的延迟抖动或数据损坏。改页表不刷 TLB 是最危险的——CPU 可能访问已释放的物理页，导致数据损坏（延迟抖动只是最好情况）。不遵循 BBM 在多核 HFT 中可能导致交易核读到管理核已释放的内存。

### HFT TLB 安全建议

```c
// HFT 最佳实践：启动后静态映射
// 1. 启动时建立所有映射
// 2. 运行时不修改页表
// 3. 不调用 mprotect/munmap
// 4. 线程绑定 CPU
// 5. 使用大页减少 TLB 压力

// 如果必须修改页表（管理核）：
void safe_page_table_update(pte_t *pte, unsigned long va, pte_t new) {
    // BBM
    *pte = 0;                                    // break
    asm volatile("dc cvac, %0" :: "r"(pte));     // clean D-cache
    asm volatile("tlbi vae1is, %0" :: "r"(va));  // flush（广播）
    asm volatile("dsb sy" ::: "memory");          // 等待
    *pte = new;                                  // make
    asm volatile("dc cvac, %0" :: "r"(pte));     // clean D-cache
    asm volatile("tlbi vae1is, %0" :: "r"(va));  // flush（可选）
    asm volatile("dsb sy" ::: "memory");
    asm volatile("isb" ::: "memory");
}
```

建议 HFT 系统启动后不再修改页表（静态映射），避免所有 TLB 相关问题。

## 自测题

1. **修改页表后不刷 TLB 会怎样？为什么？**

<details>
<summary>答案</summary>

CPU 用 TLB 中的**旧映射**翻译 VA → 访问**错误的 PA**。因为 TLB 是页表的 cache，修改页表后 TLB 中的旧条目不会自动更新。后果：访问已释放/已修改权限的物理页 → 数据损坏或 Permission fault。修复：修改页表后 `tlbi vae1is`（精确刷）或 `tlbi alle1is`（全刷）。
</details>

2. **TLBI 后不跟 DSB/ISB 会有什么问题？**

<details>
<summary>答案</summary>

- 不跟 **DSB**：TLB 刷新是异步的，DSB 等待刷新完成。不跟 DSB → 刷新可能还没完成就继续执行 → 后续访存可能仍用旧 TLB 条目。
- 不跟 **ISB**：流水线中可能有旧 TLB 翻译的指令在执行。ISB 冲刷流水线确保后续指令用新 TLB。不跟 ISB → 流水线中旧指令行为不可预测。

正确序列：`TLBI` → `DSB` → `ISB`。
</details>

3. **多核下修改页表不遵循 BBM 会导致什么问题？**

<details>
<summary>答案</summary>

核 A 直接改页表（旧 PA→新 PA），在 TLB 刷新前，核 B 可能仍用旧 TLB 条目 → 访问**旧 PA**。如果旧 PA 已被释放并分配给其他用途 → **数据损坏**或**信息泄漏**。BBM 要求先失效旧映射（break）→ 刷 TLB → DSB 等完成 → 再写新映射（make），确保所有核都不再用旧映射后才建立新映射。
</details>

4. **多核系统中 TLBI 指令忘加 `is` 后缀会怎样？**

<details>
<summary>答案</summary>

不加 `is` 的 TLBI 只刷新**本核** TLB，其他核的旧 TLB 条目不受影响。其他核可能继续用旧映射翻译 VA → 访问错误物理页。多核场景必须用 `is` 变体（如 `vae1is`），广播到所有 Inner Shareable 域的核。单核系统或不影响其他核的修改可以不加 `is`。
</details>

## 参考与延伸

- [§17.3 TLB 刷新指令](03-tlb-flush.md) — TLBI + DSB + ISB 序列
- [§17.4 BBM](04-bbm.md) — BBM 协议详解
- [Ch14 §14.8 易错点](../../chapter-14-memory-management/notes/section-0-本章完整概述.md) — MMU 相关错误
