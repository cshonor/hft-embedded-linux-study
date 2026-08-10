# §17.3 TLB 刷新指令

> **来源：** [Ch17 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

TLB 刷新指令（TLBI）：alle1（全刷）、aside1（按 ASID 刷）、vae1（按 VA 刷）、alle1is（跨核刷）。本节详解每条指令的作用、使用场景、修改页表后的 TLB 刷新流程，以及全刷 vs 精确刷的性能对比。

## 核心要点

### TLBI 指令集

| 指令 | 作用 | 粒度 | 影响范围 |
|------|------|------|---------|
| `TLBI alle1` | 刷 EL1 的所有 TLB（所有 ASID） | 全部 | 本核 |
| `TLBI aside1, x0` | 刷指定 ASID 的 TLB | 按 ASID | 本核 |
| `TLBI vae1, x0` | 刷指定 VA 的 TLB 条目 | 按 VA | 本核 |
| `TLBI alle1is` | 刷所有核的 EL1 TLB（Inner Shareable） | 全部 | **所有核** |
| `TLBI aside1is, x0` | 刷所有核指定 ASID 的 TLB | 按 ASID | **所有核** |
| `TLBI vae1is, x0` | 刷所有核指定 VA 的 TLB | 按 VA | **所有核** |
| `TLBI vmalle1` | 刷 EL1 全部（VA+ASID） | 全部 | 本核 |
| `TLBI vmalle1is` | 刷所有核 EL1 全部 | 全部 | **所有核** |

> `is` 后缀 = Inner Shareable，表示该操作广播到所有 Inner Shareable 域内的核。
> 多核系统中修改页表后必须用 `is` 变体刷新其他核的 TLB。

### 指令后缀含义

| 后缀 | 全称 | 作用 |
|------|------|------|
| 无 | — | 只刷本核 |
| `is` | Inner Shareable | 广播到同 Inner Shareable 域的所有核 |
| `os` | Outer Shareable | 广播到同 Outer Shareable 域的所有核 |

### 修改页表后的 TLB 刷新

```asm
// 方式1：全刷（简单但慢）
tlbi alle1is        // 广播到所有核
dsb sy              // 等待所有核完成刷新
isb                 // 冲刷流水线

// 方式2：按 ASID 刷（中等精度）
ldr x0, =current_asid
tlbi aside1is, x0   // 只刷该进程的所有 VA
dsb sy
isb

// 方式3：只刷指定 VA（精确）
ldr x0, =faulty_va
tlbi vae1is, x0     // 只刷该 VA 的 TLB 条目
dsb sy
isb
```

> **修改页表后必须刷 TLB**：否则 CPU 用旧的 TLB 条目翻译 → 访问错误地址。
> **DSB + ISB** 必须跟在 TLBI 后面：DSB 等刷完成，ISB 确保后续指令用新映射。

### TLBI 使用决策表

| 场景 | 推荐指令 | 原因 |
|------|---------|------|
| 进程退出 | `aside1is` | 只刷该进程的 TLB，影响最小 |
| munmap 单页 | `vae1is` | 只刷被取消映射的 VA |
| munmap 多页 | `vae1is` 循环 | 逐页刷，比全刷好 |
| 内核页表全局更新 | `alle1is` | 影响所有进程，必须全刷 |
| KVM guest 切换 | `vmalle1is` | 刷全部（含 ASID） |
| 单核修改页表 | 无 `is` 变体 | 只刷本核 |
| 多核修改页表 | `is` 变体 | 必须广播到所有核 |

### 全刷 vs 精确刷

| 方式 | 指令 | 优点 | 缺点 | 后续 miss |
|------|------|------|------|----------|
| 全刷 | `alle1is` | 简单 | 破坏所有 TLB | 大量 miss |
| 按 ASID | `aside1is` | 只影响一个进程 | 仍刷该进程全部 VA | 该进程全部 miss |
| 按 VA | `vae1is` | 最精确，只刷一个页 | 多页需多次调用 | 只一个页 miss |

### TLBI + DSB + ISB 序列详解

```asm
// 完整的页表修改 + TLB 刷新序列
// Step 1: 修改页表项
str x1, [x0]         // x0 = PTE 地址, x1 = 新 PTE 值

// Step 2: 刷 TLB（广播到所有核）
tlbi vae1is, x2      // x2 = 被修改的 VA

// Step 3: DSB 等待 TLB 刷新完成（所有核）
dsb sy               // 必须在 TLBI 之后

// Step 4: ISB 确保本核流水线清空
isb                  // 后续指令用新 TLB 映射

// 安全：现在可以释放/重用该物理页了
```

| 步骤 | 指令 | 作用 | 漏掉后果 |
|------|------|------|---------|
| 1 | STR | 写入新 PTE | — |
| 2 | TLBI | 刷新旧 TLB 条目 | CPU 用旧映射访问 |
| 3 | DSB | 等待刷新完成 | 刷新未完成就继续 |
| 4 | ISB | 冲刷流水线 | 流水线有旧翻译 |
| 5 | 释放物理页 | 安全回收 | 必须在 DSB 之后 |

## HFT 关联

TLB 刷新是 HFT 延迟抖动的来源之一——`tlbi alle1is` 会让后续大量访存 TLB miss（每次 miss ~200-400ns）。

### HFT TLB 刷新优化

```c
// HFT 系统应避免在交易路径上修改页表
// 如果必须修改，用 vae1is 精确刷新

// 错误：全刷 TLB（影响所有核的所有进程）
// tlbi alle1is  → 交易核也会 TLB miss

// 正确：只刷指定 VA（影响最小）
// tlbi vae1is, va  → 只影响一个页

// 最佳：启动后静态映射，不修改页表
// 所有内存在启动时映射好，运行时零 TLB 刷新
```

HFT 系统应避免在交易路径上修改页表。如果必须修改，用 `vae1is` 精确刷新（只影响一个页）而非 `alle1is`。在多核 HFT 系统中，`alle1is`（Inner Shareable）会触发其他核的 TLB shootdown，延迟更大。Linux 的 `mprotect`/`munmap` 会触发 TLB 刷新，HFT 应避免在交易时调用这些系统调用。

## 自测题

1. **修改页表后为什么要刷 TLB？不刷会怎样？**

<details>
<summary>答案</summary>

TLB 缓存了旧的 VA→PA 映射。修改页表后如果不刷 TLB，CPU 仍用旧映射翻译 VA → 访问**错误的 PA**（可能是已释放的物理页、已修改权限的页面等）。后果：数据损坏、权限绕过、访问已释放内存。必须刷 TLB 让 CPU 重新从新页表加载映射。
</details>

2. **TLBI 后必须跟什么指令？为什么？**

<details>
<summary>答案</summary>

必须跟 **DSB + ISB**。DSB 等待 TLB 刷新完成（TLB 刷新是异步的，需要时间传播）；ISB 冲刷流水线，确保后续指令使用新的 TLB 映射（流水线中可能有旧 TLB 翻译的指令）。不跟 DSB → 刷新未完成就继续执行；不跟 ISB → 流水线中有旧映射。
</details>

3. **`tlbi alle1is` 和 `tlbi vae1is` 在性能上有什么区别？HFT 应该用哪个？**

<details>
<summary>答案</summary>

- `alle1is`：刷**全部** TLB（所有核），后续所有访存都 TLB miss（大量 ~200ns 延迟），性能差
- `vae1is`：只刷**一个 VA** 的 TLB 条目（所有核），其他条目保留，性能好

HFT 应该用 `vae1is` 精确刷新，避免全刷导致的 TLB rebuild 开销。但 `vae1is` 只能刷一个页，多个页需要多次调用。
</details>

4. **`is` 后缀的 TLBI 指令和不带 `is` 的有什么区别？什么时候必须用 `is`？**

<details>
<summary>答案</summary>

- 不带 `is`：只刷**本核** TLB
- 带 `is`（Inner Shareable）：广播到**所有核**的 TLB

多核系统中修改页表后**必须用 `is` 变体**。因为其他核的 TLB 也缓存了旧映射，只刷本核会导致其他核继续用旧 TLB → 访问错误物理页。单核系统或确认只有本核使用该映射时可以不带 `is`。
</details>

## 参考与延伸

- [§17.2 ASID](02-asid.md) — aside1 指令的 ASID 参数
- [§17.4 BBM](04-bbm.md) — 修改页表的安全协议
- [Ch18 §18.3 典型场景](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md) — TLB 维护的屏障序列
