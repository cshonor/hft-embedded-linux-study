# §17.4 BBM（Break-Before-Make）

> **来源：** [Ch17 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

修改页表项（从有效→有效，改映射）时必须遵循 BBM 协议：先失效旧映射（break）→ TLB 刷新 → DSB → 写新映射（make）。不遵循可能导致多核下新旧映射同时可见，数据损坏。

## 核心要点

### BBM 步骤

```
1. 将页表项设为 Invalid（break）
2. TLB 刷新（确保旧映射失效）
3. DSB（等待 TLB 刷新完成）
4. 写入新的有效映射（make）
5. TLB 刷新（可选，确保新映射可见）
6. DSB + ISB
```

### 为什么需要 BBM？

| 不遵循 BBM | 后果 |
|-----------|------|
| 直接改页表项（旧→新） | 在 TLB 刷新之前，其他核可能用旧 TLB → 访问旧 PA |
| 旧 PA 已被释放/重用 | 数据损坏或信息泄漏 |

> 不遵循 BBM → 在 break 和 make 之间，其他核可能用旧 TLB → 访问已释放的物理页 → 数据损坏。
> 内核的 `set_pte()` 通常封装了 BBM 逻辑。

### BBM 适用场景

| 场景 | 需要 BBM？ | 原因 |
|------|-----------|------|
| Invalid → Valid（新映射） | 不需要 | 没有旧映射需要 break |
| Valid → Valid（改映射） | **需要** | 旧映射可能还在 TLB 中 |
| Valid → Invalid（取消映射） | 不需要 | 只 break 不 make |

## HFT 关联

BBM 在 HFT 多核系统中很重要——如果交易核和管理核共享页表，管理核修改映射时必须遵循 BBM，否则交易核可能用旧 TLB 访问已释放的物理页。在裸金属 HFT 中，通常不动态修改页表（启动时建好映射后不再变），BBM 不是日常问题。但如果需要动态映射内存（如运行时分配大页），必须遵循 BBM。Linux 的 `set_pte_at()` 已封装 BBM，不需要手动处理。

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

## 参考与延伸

- [§17.3 TLB 刷新指令](03-tlb-flush.md) — BBM 中使用的 TLBI 指令
- [§17.7 易错点](07-pitfalls.md) — BBM 相关错误
- [Ch18 §18.2 三条屏障指令](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md) — DSB 在 BBM 中的作用
