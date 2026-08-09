# Ch 3 §2 遍历与使用页表 (Using Page Table Entries)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**

### 2. 遍历与使用页表 (Using Page Table Entries)

内核频繁 **页表遍历 (page table walk)** — 缺页、munmap、mprotect、swap、debugger 等。

### 定位各级表项

| 宏（原书） | 作用 |
|------------|------|
| **`pgd_offset()`** | 由线性地址 + `mm` 得 **PGD 项** |
| **`pmd_offset()`** | 得 **PMD 项** |
| **`pte_offset()`** | 得 **PTE 项** |

（现代树在深层 arch 上可能先经 **`p4d_offset` / `pud_offset`**。）

### 检查 / 修改 PTE 状态

| 宏 | 作用 |
|----|------|
| **`pte_present()`** | 是否在内存 |
| **`pte_dirty()` / `pte_mkdirty()`** | 读/设 dirty |
| **`pte_young()` / `pte_mkyoung()`** | 读/设 accessed（young） |
| **`set_pte()` / `pte_clear()`** | 安装 / 清除映射 |

**HFT：** 热路径 **用户态** 不跑这些宏；但 **prefault、mlock、大页合并/分裂** 会在内核里 **批量改 PTE** → 触发 **TLB shootdown**。

---
