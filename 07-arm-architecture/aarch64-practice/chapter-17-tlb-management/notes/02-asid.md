# §17.2 ASID（Address Space ID）

> **来源：** [Ch17 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

ASID 区分不同进程的 TLB 条目，切换进程时只换 ASID 不刷 TLB。没有 ASID 每次进程切换都要 flush 全部 TLB，性能差。

## 核心要点

### ASID 机制

| 特性 | 说明 |
|------|------|
| ASID 宽度 | 通常 8 位或 16 位（TCR_EL1.AS） |
| 作用 | TLB 条目带 ASID 标签，切换进程不刷全部 TLB |
| TCR_EL1.AS | ASID 宽度选择（0=8bit, 1=16bit） |
| TTBRx_EL1 | 高位存放 ASID |

### 有无 ASID 对比

| 场景 | 无 ASID | 有 ASID |
|------|---------|---------|
| 进程切换 | flush 全部 TLB | 只换 ASID |
| TLB 条目 | 无进程标签 | 带 ASID 标签 |
| 切回旧进程 | TLB cold（全 miss） | TLB hot（旧条目仍在） |
| 性能 | 差（频繁 TLB rebuild） | 好 |

### 设置 ASID

```asm
// 设置 ASID（进程切换时）
msr TTBR0_EL1, x0      // x0 高位包含 ASID
isb

// 带 ASID 的 TLB 刷新（只刷当前 ASID）
tlbi aside1, x0        // x0 = ASID
```

> **没有 ASID**：每次进程切换都要 flush 全部 TLB → 性能差。
> **有 ASID**：切换进程时只换 ASID，旧进程的 TLB 条目仍在（下次切回来命中）。

## HFT 关联

HFT 系统通常是单进程裸金属，不涉及进程切换，ASID 的价值不大。但如果 HFT 系统有管理进程（如监控进程），ASID 可以避免管理进程和交易进程的 TLB 互相 flush。在 Linux HFT 方案中，ASID 是默认启用的——Linux 进程切换时设置 ASID，TLB 条目跨切换保留，减少 TLB rebuild 开销。ASID 8 位只能区分 256 个进程，16 位可以 65536 个，现代 ARM 支持 16 位 ASID。

## 自测题

1. **ASID 解决什么问题？有了 ASID 后进程切换还需要刷全部 TLB 吗？**

<details>
<summary>答案</summary>

ASID 解决**进程切换时 TLB flush 的性能问题**。没有 ASID 时，每次进程切换必须 flush 全部 TLB（因为新旧进程的 VA 相同但映射不同）。有 ASID 后，TLB 条目带 ASID 标签，切换进程只换 ASID，**不需要刷 TLB**。旧进程的 TLB 条目仍在，下次切回来直接命中。
</details>

2. **ASID 存放在哪里？8 位 ASID 最多支持多少进程？**

<details>
<summary>答案</summary>

ASID 存放在 **TTBR0_EL1 的高位**（bit[63:48] 或 bit[63:56]，取决于 AS 宽度）。8 位 ASID 最多支持 **256** 个进程。16 位 ASID（TCR_EL1.AS=1）支持 65536 个。如果进程数超过 ASID 上限，内核需要回收旧 ASID 并 flush 对应 TLB。
</details>

3. **`tlbi aside1, x0` 和 `tlbi alle1` 的区别？**

<details>
<summary>答案</summary>

- `tlbi aside1, x0`：只刷**指定 ASID** 的 TLB 条目（x0 = ASID），其他 ASID 的条目保留
- `tlbi alle1`：刷**所有 ASID** 的 EL1 TLB 条目，全部清空

`aside1` 精确、影响小；`alle1` 粗暴、影响大。进程退出时用 `aside1` 只刷该进程的 TLB。
</details>

## 参考与延伸

- [§17.1 TLB 基本概念](01-tlb-basics.md) — TLB 命中/未命中
- [§17.3 TLB 刷新指令](03-tlb-flush.md) — aside1/alle1 等指令
- [§17.5 内核 TLB 维护场景](05-tlb-scenarios.md) — 进程切换的 TLB 操作
