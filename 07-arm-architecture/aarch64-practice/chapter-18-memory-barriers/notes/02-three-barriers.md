# §18.2 三条屏障指令

> **来源：** [Ch18 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

ARM 的三条屏障指令：DMB（访存有序但 CPU 不停）、DSB（完全停住等访存完成）、ISB（冲刷流水线重新取指）。各有不同的作用域（sy/ish/osh/nsh/st）。

## 核心要点

### 三条屏障指令

| 指令 | 全称 | 行为 | 强度 |
|------|------|------|------|
| **DMB** | Data Memory Barrier | 保证屏障前的访存完成后，屏障后的访存才**对外可见**。CPU 可继续执行非访存指令。 | 中 |
| **DSB** | Data Synchronization Barrier | 比 DMB 更强：屏障前的访存完成后，**才执行后续任何指令**（完全停住）。 | 强 |
| **ISB** | Instruction Synchronization Barrier | 冲刷流水线，保证后续指令**重新取指**。用于修改系统寄存器/代码后。 | 指令侧 |

### DMB vs DSB 区别

```
// DMB：访存有序，但 CPU 不停
str x1, [data]     // Store A
dmb sy             // 屏障
str x2, [flag]     // Store B 必须在 A 之后对外可见
add x3, x4, x5     // CPU 可以先执行这条（非访存）

// DSB：完全停住
str x1, [data]     // Store A
dsb sy             // 等 A 完成，且停住
add x3, x4, x5     // 必须等 DSB 完成才能执行
```

### 作用域

| 后缀 | 作用域 |
|------|--------|
| `sy` | Full system（所有可观察者） |
| `ish` | Inner Shareable（同一簇 CPU） |
| `osh` | Outer Shareable（包括 DMA） |
| `nsh` | Non-shareable（仅当前 CPU） |
| `st` | 仅 Store（不约束 Load） |

```asm
dmb sy      // 全屏障：Load+Store，全系统
dmb ishst   // 仅 Store 屏障，Inner Shareable
dsb sy      // 最强：全停，全系统
isb         // 指令同步
```

## HFT 关联

DMB 和 DSB 的选择直接影响 HFT 性能。DMB 只约束访存顺序，CPU 可以继续执行非访存指令（如算术运算），延迟更小。DSB 完全停住 CPU，延迟最大。HFT 中应尽量用 DMB 而非 DSB——只有 DMA 和 TLB 维护等必须等完成的场景才用 DSB。作用域选择也很重要：`ish`（Inner Shareable）比 `sy`（Full system）轻量，多核 CPU 间同步用 `ish` 足够。`ishst`（仅 Store）比 `ish`（Load+Store）更轻，生产者写场景只需 `ishst`。

## 自测题

1. **DMB 和 DSB 的区别是什么？什么时候必须用 DSB？**

<details>
<summary>答案</summary>

- **DMB**：保证访存顺序（屏障前的访存先于屏障后的访存对外可见），但 CPU **不停**，可以继续执行非访存指令
- **DSB**：比 DMB 更强，完全**停住 CPU**，等屏障前所有访存完成后才执行后续任何指令

**必须用 DSB 的场景**：DMA（需要数据完全写入内存后才启动 DMA）、TLB 维护（需要 TLB 刷新完成后才继续）、修改系统寄存器后。
</details>

2. **`dmb ishst` 的作用域和约束类型是什么？**

<details>
<summary>答案</summary>

- 作用域 = **ish**（Inner Shareable，同一簇 CPU 间有效）
- 约束类型 = **st**（仅 Store，不约束 Load）

即：保证屏障前的 Store 先于屏障后的 Store 对同一簇 CPU 可见。不约束 Load。这是最轻量的 Store-Store 屏障，适用于生产者写数据后写 flag 的场景。
</details>

3. **ISB 的作用是什么？什么时候需要用？**

<details>
<summary>答案</summary>

ISB 冲刷**流水线**，保证后续指令重新从 I-cache/内存取指。需要用 ISB 的场景：
1. 修改系统寄存器后（如开 MMU 后、改 SCTLR 后）
2. 修改代码后（自修改代码，配合 I-Cache invalidate）
3. TLB 刷新后（确保流水线中没有用旧 TLB 的指令）

ISB 不约束访存顺序，它只影响指令取指。
</details>

## 参考与延伸

- [§18.1 弱序内存模型](01-weak-memory-model.md) — 为什么需要屏障
- [§18.3 典型场景](03-typical-scenarios.md) — DMB/DSB 在实际场景中的选择
- [§18.5 Linux 内核屏障 API](05-linux-barrier-api.md) — smp_mb/mb 等对应关系
