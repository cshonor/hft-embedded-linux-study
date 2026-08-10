# §17.5 内核 TLB 维护场景

> **来源：** [Ch17 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

Linux 内核中各种 TLB 维护场景：进程切换（有/无 ASID）、munmap、fork COW、kmap/kunmap、模块加载。不同场景用不同的 TLBI 指令。

## 核心要点

### 场景与操作

| 场景 | 操作 | TLBI 指令 |
|------|------|-----------|
| 进程切换（无 ASID） | `TLBI alle1` 全刷 | alle1 |
| 进程切换（有 ASID） | 换 ASID，不刷 | 不需要 |
| munmap | `TLBI vae1` 刷指定 VA | vae1 |
| fork → COW | 刷新被修改的页 | vae1 |
| kmap/kunmap | `TLBI vae1` 或 `TLBI alle1` | vae1/alle1 |
| 模块加载 | 无需（代码在内核空间，TLB 命中） | 不需要 |

### 进程切换 TLB 策略

| 有 ASID | 无 ASID |
|---------|---------|
| 换 TTBR0 的 ASID 字段 | `tlbi alle1` 全刷 |
| 旧进程 TLB 保留 | 旧进程 TLB 全部失效 |
| 切回旧进程 TLB hot | 切回旧进程 TLB cold |
| 性能好 | 性能差 |

### munmap 流程

```
1. 从页表删除映射（设 Invalid）
2. TLBI vae1, vaddr  ← 刷指定 VA 的 TLB
3. DSB + ISB
4. 释放物理页
```

> 注意：必须先刷 TLB 再释放物理页，否则其他核可能用旧 TLB 访问已释放的页。

## HFT 关联

HFT 系统在 Linux 上运行时，`munmap` 是 TLB 抖动的来源——释放内存时触发 `vae1` TLB 刷新。HFT 应避免在交易路径上分配/释放内存（使用内存池）。进程切换（`schedule`）如果无 ASID 会全刷 TLB，HFT 应将交易线程绑定到专用 CPU 核（`CPU affinity`），避免被调度器切换。kmap/kunmap 在 HFT 中应避免（使用大页 permanent mapping 替代临时映射）。

## 自测题

1. **有 ASID 和无 ASID 的进程切换，TLB 操作有什么不同？**

<details>
<summary>答案</summary>

- **无 ASID**：进程切换时 `tlbi alle1` **全刷 TLB**，旧进程的 TLB 条目全部失效。切回旧进程时 TLB cold（全 miss），性能差。
- **有 ASID**：进程切换只**换 ASID**（写 TTBR0 高位），不刷 TLB。旧进程的 TLB 条目带旧 ASID 标签仍保留。切回旧进程时 TLB hot，性能好。
</details>

2. **munmap 时为什么要先刷 TLB 再释放物理页？**

<details>
<summary>答案</summary>

多核系统中，其他核可能还有该 VA 的 TLB 条目。如果先释放物理页再刷 TLB：其他核可能用旧 TLB 访问已释放的物理页 → **use-after-free**（数据损坏或信息泄漏）。必须先 `tlbi vae1` 刷 TLB + DSB 等完成，确保所有核都不再用旧映射，然后才释放物理页。这就是 BBM 的思想。
</details>

3. **模块加载为什么不需要刷 TLB？**

<details>
<summary>答案</summary>

模块加载到**内核空间**（高地址 TTBR1），内核页表在所有进程间共享，TLB 条目在所有进程间有效。模块代码所在的虚拟地址在 TLB 中已有映射（内核空间通常用大块映射），不需要新增页表项。但如果模块加载涉及新分配内核页（如 vmalloc 区域），则需要刷 TLB。
</details>

## 参考与延伸

- [§17.2 ASID](02-asid.md) — ASID 机制详解
- [§17.3 TLB 刷新指令](03-tlb-flush.md) — 各种 TLBI 指令
- [§17.4 BBM](04-bbm.md) — munmap 中的 BBM
