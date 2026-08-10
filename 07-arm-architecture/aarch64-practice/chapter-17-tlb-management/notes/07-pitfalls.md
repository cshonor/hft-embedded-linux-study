# §17.7 易错点清单

> **来源：** [Ch17 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

TLB 管理的 4 个常见错误：改页表不刷 TLB、TLBI 后不跟 DSB/ISB、不用 ASID 全刷 TLB、不遵循 BBM。

## 核心要点

| # | 易错点 | 后果 | 修复 |
|---|--------|------|------|
| 1 | 改页表不刷 TLB | CPU 用旧映射，访问错误物理页 | 改页表后必须 TLBI |
| 2 | TLBI 后不跟 DSB/ISB | 刷新未完成就继续执行，行为未定义 | TLBI → DSB → ISB |
| 3 | 不用 ASID 全刷 TLB | 进程切换性能差（TLB cold miss 暴增） | 启用 ASID，按 ASID 刷 |
| 4 | 不遵循 BBM | 多核下可能同时看到新旧映射 → 数据损坏 | break → flush → DSB → make |

### 调试技巧

| 症状 | 可能原因 |
|------|----------|
| 修改页表后访问错误地址 | 没刷 TLB（用旧映射） |
| TLBI 后行为异常 | 没跟 DSB/ISB |
| 进程切换后性能暴跌 | 无 ASID，全刷 TLB |
| 多核数据损坏 | 没遵循 BBM |

## HFT 关联

TLB 错误在 HFT 系统中通常导致不可预测的延迟抖动或数据损坏。改页表不刷 TLB 是最危险的——CPU 可能访问已释放的物理页，导致数据损坏（延迟抖动只是最好情况）。不遵循 BBM 在多核 HFT 中可能导致交易核读到管理核已释放的内存。建议 HFT 系统启动后不再修改页表（静态映射），避免所有 TLB 相关问题。

## 自测题

1. **修改页表后不刷 TLB 会怎样？为什么？**

<details>
<summary>答案</summary>

CPU 用 TLB 中的**旧映射**翻译 VA → 访问**错误的 PA**。因为 TLB 是页表的 cache，修改页表后 TLB 中的旧条目不会自动更新。后果：访问已释放/已修改权限的物理页 → 数据损坏或 Permission fault。修复：修改页表后 `tlbi vae1`（精确刷）或 `tlbi alle1`（全刷）。
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

## 参考与延伸

- [§17.3 TLB 刷新指令](03-tlb-flush.md) — TLBI + DSB + ISB 序列
- [§17.4 BBM](04-bbm.md) — BBM 协议详解
- [Ch14 §14.8 易错点](../../chapter-14-memory-management/notes/section-0-本章完整概述.md) — MMU 相关错误
