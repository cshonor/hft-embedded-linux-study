# 5 级页表

> **原文:** [Five-level page tables](https://lwn.net/Articles/717293/) (LWN, 2017)
> **内核版本:** 4.14+ (x86), 5.x+ (ARM64)
> **对标旧书:** ULK3 Ch2 / LKD3 Ch3 (4 级页表)

---

## 核心观点

5 级页表将虚拟地址空间从 48 位 (256TB) 扩展到 57 位 (128PB)，适应大内存服务器需求。

### 页表层级对比

```
4 级页表 (48-bit VA, 256TB):
  PGD (9b) → PUD (9b) → PMD (9b) → PTE (9b) → offset (12b)
  每级 512 个条目, 页大小 4KB

5 级页表 (57-bit VA, 128PB):
  PGD (9b) → P4D (9b) → PUD (9b) → PMD (9b) → PTE (9b) → offset (12b)
  新增 P4D 层, 每级 512 个条目, 页大小 4KB
```

### 虚拟地址空间布局

```
4 级 (48-bit):
  用户空间: 0x0000_0000_0000_0000 ~ 0x0000_7FFF_FFFF_FFFF (128TB)
  内核空间: 0xFFFF_8000_0000_0000 ~ 0xFFFF_FFFF_FFFF_FFFF (128TB)

5 级 (57-bit):
  用户空间: 0x0000_0000_0000_0000 ~ 0x00FF_FFFF_FFFF_FFFF (64PB)
  内核空间: 0xFF00_0000_0000_0000 ~ 0xFFFF_FFFF_FFFF_FFFF (64PB)
```

### 启用条件

```bash
# x86: 需要 CPU 支持 LA57 (Intel Ice Lake+)
# 启动参数: no5lvl (禁用) / 默认自动启用

# ARM64: 需要 CPU 支持 ARMv8.2 LVA (Large Virtual Addressing)
# 内核配置: CONFIG_ARM64_64K_PAGES=y (64KB 页 + 3 级 = 42-bit, 不需要 5 级)
# 或 CONFIG_PGTABLE_LEVELS=5

# 检查当前页表层级
grep page_table_levels /proc/cpuinfo  # x86
cat /proc/vmstat | grep page_table     # 间接判断
```

---

## 与旧书差异

| ULK3 / LKD3 讲的 | 现代实现 |
|-------------------|---------|
| 4 级页表 (48-bit) | 5 级页表 (57-bit, 可选) |
| 无 P4D 层 | 新增 P4D 层 |
| 256TB 虚拟空间 | 128PB 虚拟空间 (5 级) |
| `pgd_offset()` → `pud_offset()` | `pgd_offset()` → `p4d_offset()` → `pud_offset()` |

---

## HFT 关联

5 级页表对 HFT 通常不利：(1) 多一级页表意味着 TLB miss 时多一次内存访问（~100ns）；(2) HFT 虚拟地址空间不大，不需要 128PB。建议 HFT 系统使用 `no5lvl` 禁用 5 级页表，保持 4 级。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 5 级页表对 TLB miss 代价有什么影响？

> 4 级页表 TLB miss 需要 4 次内存访问（PGD→PUD→PMD→PTE）。5 级增加为 5 次（PGD→P4D→PUD→PMD→PTE），多一次 ~100ns。对于 TLB miss 率高的应用（大工作集），总延迟增加显著。HFT 通常工作集小 + 大页，TLB miss 率低，影响有限但仍建议禁用。

**Q2:** 内核如何兼容 4 级和 5 级页表？

> 通过 `CONFIG_PGTABLE_LEVELS` 编译时选择 + `pgtable_fold` 运行时折叠。当配置为 5 级但 CPU 不支持 LA57 时，P4D 层被"折叠"（pgd 等于 p4d，p4d_offset 直接返回 pgd）。这样代码统一写 5 级路径，折叠后实际走 4 级，无需 `#ifdef`。

</details>
