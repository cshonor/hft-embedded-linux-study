# §18.2 三条屏障指令

> **来源：** [Ch18 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

ARM 的三条屏障指令：DMB（访存有序但 CPU 不停）、DSB（完全停住等访存完成）、ISB（冲刷流水线重新取指）。各有不同的作用域（sy/ish/osh/nsh/st）。本节详解每条指令的行为、作用域选择和使用场景。

## 核心要点

### 三条屏障指令

| 指令 | 全称 | 行为 | 强度 | 用途 |
|------|------|------|------|------|
| **DMB** | Data Memory Barrier | 屏障前的访存完成后，屏障后的访存才对外可见。CPU 可继续执行非访存指令。 | 中 | CPU 间访存排序 |
| **DSB** | Data Synchronization Barrier | 比 DMB 更强：屏障前访存完成后，才执行后续**任何**指令（完全停住）。 | 强 | DMA/TLB/系统寄存器 |
| **ISB** | Instruction Synchronization Barrier | 冲刷流水线，保证后续指令重新取指。 | 指令侧 | 修改系统寄存器/代码后 |

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

| 后缀 | 作用域 | 典型场景 |
|------|--------|---------|
| `sy` | Full system（所有可观察者） | DMA、全局同步 |
| `ish` | Inner Shareable（同一簇 CPU） | 多核 CPU 间同步 |
| `osh` | Outer Shareable（包括 DMA） | CPU↔DMA 同步 |
| `nsh` | Non-shareable（仅当前 CPU） | 单核内同步 |
| `st` | 仅 Store（不约束 Load） | 生产者写场景 |

### 作用域 + 约束类型组合

| 指令 | 作用域 | 约束 | 典型延迟 | 场景 |
|------|--------|------|---------|------|
| `dmb sy` | 全系统 | Load+Store | ~10-15ns | 最强全屏障 |
| `dmb ish` | Inner Shareable | Load+Store | ~5-10ns | CPU 间全屏障 |
| `dmb ishst` | Inner Shareable | 仅 Store | ~3-5ns | 生产者写屏障 |
| `dmb ishld` | Inner Shareable | 仅 Load | ~3-5ns | 消费者读屏障 |
| `dsb sy` | 全系统 | 完全停住 | ~50-100ns | DMA 同步 |
| `dsb ish` | Inner Shareable | 完全停住 | ~30-50ns | TLB 刷新 |
| `isb` | 本核 | 流水线冲刷 | ~10-20ns | 系统寄存器修改 |

```asm
dmb sy      // 全屏障：Load+Store，全系统
dmb ishst   // 仅 Store 屏障，Inner Shareable（最常用！）
dmb ishld   // 仅 Load 屏障，Inner Shareable
dsb sy      // 最强：全停，全系统
isb         // 指令同步
```

### ISB 的特殊用途

```asm
// 场景1：修改系统寄存器后
msr SCTLR_EL1, x0    // 开 MMU
isb                   // 确保后续指令在 MMU 开启后执行

// 场景2：修改代码后（自修改代码）
dc cvau, addr         // clean D-cache
ic ivau, addr         // invalidate I-cache
dsb sy
isb                   // 重新取指，执行新代码

// 场景3：TLB 刷新后
tlbi alle1
dsb ish
isb                   // 确保后续指令用新 TLB 映射
```

### 屏障选择速查

| 需求 | 指令 | 说明 |
|------|------|------|
| CPU 间 Store-Store 有序 | `dmb ishst` | 最轻量，生产者写 |
| CPU 间 Load-Load 有序 | `dmb ishld` | 消费者读 |
| CPU 间全屏障 | `dmb ish` | 自旋锁 |
| DMA 前后 | `dsb sy` | 完全停住 |
| TLB 刷新后 | `dsb ish` + `isb` | 等 TLB + 重取指 |
| 修改系统寄存器 | `isb` | 流水线冲刷 |
| 修改代码后 | `dsb sy` + `isb` | D-cache clean + I-cache inval |

## HFT 关联

DMB 和 DSB 的选择直接影响 HFT 性能。DMB 只约束访存顺序，CPU 可以继续执行非访存指令（如算术运算），延迟更小。DSB 完全停住 CPU，延迟最大。

### HFT 屏障选择原则

```c
// 原则1：用 DMB 不用 DSB（除非必须停住）
dmb ishst   // ✓ ~3-5ns
dsb sy      // ✗ ~50-100ns，除非 DMA/TLB

// 原则2：用 ish 不用 sy（除非涉及 DMA）
dmb ish     // ✓ Inner Shareable，多核间足够
dmb sy      // ✗ Full system，更重

// 原则3：用 ishst/ishld 不用 ish（只需 Store/Load）
dmb ishst   // ✓ 仅 Store，生产者
dmb ish     // ✗ Store+Load，过度

// 原则4：用 LDAR/STLR 不用 DMB（最优）
stlr w0, [addr]  // ✓ Store-Release，~5ns
str w0, [addr]
dmb ishst        // ✗ ~8ns，两步
```

HFT 中应尽量用 DMB 而非 DSB——只有 DMA 和 TLB 维护等必须等完成的场景才用 DSB。作用域选择也很重要：`ish`（Inner Shareable）比 `sy`（Full system）轻量，多核 CPU 间同步用 `ish` 足够。`ishst`（仅 Store）比 `ish`（Load+Store）更轻，生产者写场景只需 `ishst`。

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

4. **HFT 中需要 CPU 间 Store-Store 有序，应该选哪个屏障？为什么不用 `dsb sy`？**

<details>
<summary>答案</summary>

选 `dmb ishst`。原因：
1. **DMB 不 DSB**：只需访存顺序，不需要停住 CPU，DMB 更轻（~3-5ns vs ~50-100ns）
2. **ish 不 sy**：CPU 间同步只需 Inner Shareable，不需要全系统（sy 更重）
3. **st 不全**：只需约束 Store-Store，不需要约束 Load

`dsb sy` 是最强屏障（完全停住 + 全系统），过度使用会严重降低性能。
</details>

## 参考与延伸

- [§18.1 弱序内存模型](01-weak-memory-model.md) — 为什么需要屏障
- [§18.3 典型场景](03-typical-scenarios.md) — DMB/DSB 在实际场景中的选择
- [§18.5 Linux 内核屏障 API](05-linux-barrier-api.md) — smp_mb/mb 等对应关系
