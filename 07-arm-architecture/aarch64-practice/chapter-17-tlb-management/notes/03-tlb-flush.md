# §17.3 TLB 刷新指令

> **来源：** [Ch17 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

TLB 刷新指令（TLBI）：alle1（全刷）、aside1（按 ASID 刷）、vae1（按 VA 刷）、alle1is（跨核刷）。修改页表后必须刷 TLB，且 TLBI 后必须跟 DSB + ISB。

## 核心要点

### TLBI 指令集

| 指令 | 作用 |
|------|------|
| `TLBI alle1` | 刷 EL1 的所有 TLB（所有 ASID） |
| `TLBI aside1, x0` | 刷指定 ASID 的 TLB |
| `TLBI vae1, x0` | 刷指定 VA 的 TLB 条目 |
| `TLBI alle1is` | 刷所有核的 EL1 TLB（Inner Shareable） |
| `TLBI vmalle1` | 刷 EL1 全部（VA+ASID） |

### 修改页表后的 TLB 刷新

```asm
// 方式1：全刷（简单但慢）
tlbi alle1
dsb sy
isb

// 方式2：只刷指定 VA（精确）
ldr x0, =faulty_va
tlbi vae1, x0
dsb sy
isb
```

> **修改页表后必须刷 TLB**：否则 CPU 用旧的 TLB 条目翻译 → 访问错误地址。
> **DSB + ISB** 必须跟在 TLBI 后面：DSB 等刷完成，ISB 确保后续指令用新映射。

### 全刷 vs 精确刷

| 方式 | 指令 | 优点 | 缺点 |
|------|------|------|------|
| 全刷 | `alle1` | 简单 | 破坏所有 TLB，后续大量 miss |
| 按 ASID | `aside1` | 只影响一个进程 | 仍刷该进程全部 VA |
| 按 VA | `vae1` | 最精确，只刷一个页 | 只能刷一个 VA，多个页需多次 |

## HFT 关联

TLB 刷新是 HFT 延迟抖动的来源之一——`tlbi alle1` 会让后续大量访存 TLB miss（每次 miss ~200-400ns）。HFT 系统应避免在交易路径上修改页表。如果必须修改，用 `vae1` 精确刷新（只影响一个页）而非 `alle1`。在多核 HFT 系统中，`alle1is`（Inner Shareable）会触发其他核的 TLB shootdown，延迟更大。Linux 的 `mprotect`/`munmap` 会触发 TLB 刷新，HFT 应避免在交易时调用这些系统调用。

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

3. **`tlbi alle1` 和 `tlbi vae1` 在性能上有什么区别？HFT 应该用哪个？**

<details>
<summary>答案</summary>

- `alle1`：刷**全部** TLB，后续所有访存都 TLB miss（大量 ~200ns 延迟），性能差
- `vae1`：只刷**一个 VA** 的 TLB 条目，其他条目保留，性能好

HFT 应该用 `vae1` 精确刷新，避免全刷导致的 TLB rebuild 开销。但 `vae1` 只能刷一个页，多个页需要多次调用。
</details>

## 参考与延伸

- [§17.2 ASID](02-asid.md) — aside1 指令的 ASID 参数
- [§17.4 BBM](04-bbm.md) — 修改页表的安全协议
- [Ch18 §18.3 典型场景](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md) — TLB 维护的屏障序列
