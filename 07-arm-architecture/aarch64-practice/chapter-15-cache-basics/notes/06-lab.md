# §15.6 实验要点

> **来源：** [Ch15 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

本章 2 个实验：枚举高速缓存（读 CTR_EL0）和清理高速缓存（dc cvac/civac）。在 Linux 用户态操作 cache 系统寄存器。

## 核心要点

| 实验 | 内容 | 平台 | 关键技能 |
|------|------|------|----------|
| 15-1 | 枚举高速缓存（读 CTR_EL0 / CSI） | Linux | 读 cache 信息寄存器 |
| 15-2 | 清理高速缓存（dc cvac / civac） | Linux | cache 维护指令 |

### 实验 15-1 关键寄存器

```c
// 读 CTR_EL0 获取 L1 cache 信息
u64 ctr;
asm volatile("mrs %0, CTR_EL0" : "=r"(ctr));
// bit[3:0]  = DminLine（D-cache line 最小大小，2^n 字节）
// bit[19:16]= IminLine（I-cache line 最小大小）
// bit[31]   = DIC（I/D cache 独立性）
// bit[30]   = IDC（I/D cache 一致性）
```

### 实验 15-2 关键指令

```asm
// Clean（写回脏数据）
dc cvac, x0       // Clean by VA to PoC
// Clean + Invalidate（写回+丢弃）
dc civac, x0      // Clean+Invalidate by VA to PoC
// Invalidate（丢弃，不写回）
dc ivac, x0       // Invalidate by VA to PoC
```

## HFT 关联

实验 15-1 的 CTR_EL0 寄存器在 HFT 中用于动态获取 cache line 大小——代码不应硬编码 64 字节，而应从 CTR_EL0 读取，确保在不同平台上正确对齐。实验 15-2 的 cache 维护指令在 HFT DMA 场景中必需。理解 `dc cvac`（Clean）和 `dc civac`（Flush）的区别可以避免 DMA 数据不一致 bug。

## 自测题

1. **如何用 CTR_EL0 获取 D-cache line 大小？**

<details>
<summary>答案</summary>

```c
u64 ctr;
asm volatile("mrs %0, CTR_EL0" : "=r"(ctr));
int dminline = ctr & 0xF;        // bit[3:0]
int cache_line_size = 4 << dminline;  // 2^(dminline+2) 字节
```
如 dminline=4 → line size = 4 << 4 = 64 字节。
</details>

2. **`dc cvac` 和 `dc civac` 的区别是什么？分别用于什么场景？**

<details>
<summary>答案</summary>

- `dc cvac`：**Clean**（只写回脏数据，不丢弃 cache 行）→ 用于 DMA 读内存前（内存→设备）
- `dc civac`：**Clean + Invalidate**（写回+丢弃）→ 用于自修改代码或需要确保下次从内存读的场景

cvac 后 cache 行仍在，后续可能命中；civac 后 cache 行被丢弃，下次必定 miss。
</details>

3. **在 Linux 用户态可以直接执行 `dc cvac` 吗？有什么限制？**

<details>
<summary>答案</summary>

在 Linux 用户态（EL0）**可以执行** `dc cvac`（ARMv8 允许 EL0 操作 cache），但只能操作自己进程地址空间的 VA。CTR_EL0 也是 EL0 可读的。但 `dc ivac`（Invalidate）通常需要 EL1 权限。实验在 Linux 用户态跑需要内核模块或用 `/dev/mem` 映射物理内存。
</details>

## 参考与延伸

- [§15.4 关键概念](04-key-concepts.md) — Clean/Invalidate/Flush 定义
- [§15.5 DMA 与 Cache](05-dma-cache.md) — cache 维护指令在 DMA 中的使用
- [§15.7 易错点](07-pitfalls.md) — 实验中的常见错误
