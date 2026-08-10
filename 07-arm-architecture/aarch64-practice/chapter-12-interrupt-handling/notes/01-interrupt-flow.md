# §12.1 中断处理全流程

> **来源：** [Ch12 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

从硬件产生中断信号到 ERET 返回的完整 IRQ 处理流程：中断源 → GIC 仲裁 → CPU IRQ 线 → 硬件保存 → 向量表跳转 → 软件保存 → ISR → EOIR → 恢复 → ERET。

## 完整流程

```
硬件外设产生中断信号
  ↓
GIC（中断控制器）接收、仲裁、优先级排序
  ↓
GIC 将最高优先级中断路由到目标 CPU 的 IRQ 线
  ↓
CPU IRQ 线拉高，CPU 在当前指令执行完后响应
  ↓
CPU 硬件自动保存：ELR_ELx←PC, SPSR_ELx←PSTATE
  ↓
CPU 硬件设置 PSTATE（屏蔽 DAIF，切到目标 EL/SP）
  ↓
CPU 跳转到 VBAR_ELx + offset（如 0x280 内核态 IRQ）
  ↓
软件保存 X0-X30（STP 压栈）
  ↓
软件读 GIC IAR（中断确认寄存器）→ 获取中断号
  ↓
软件根据中断号调用对应 ISR
  ↓
ISR 处理中断（读外设数据、清中断标志等）
  ↓
软件写 GIC EOIR（结束中断）
  ↓
软件恢复 X0-X30（LDP 恢复）
  ↓
ERET 返回（ELR→PC, SPSR→PSTATE）
```

## 关键阶段详解

| 阶段 | 执行者 | 操作 | 延迟 |
|------|--------|------|------|
| 中断信号 | 硬件外设 | 产生电平/边沿信号 | ~0 |
| 仲裁路由 | GIC | 优先级排序、路由到目标 CPU | ~50-100ns |
| CPU 响应 | CPU 硬件 | 当前指令完成后响应 IRQ | 1-10ns |
| 硬件保存 | CPU 硬件 | ELR+SPSR 自动保存，切 SP | ~10ns |
| 向量跳转 | CPU 硬件 | PC = VBAR + offset | ~5ns |
| 软件保存 | 软件 | STP X0-X30 到栈 | ~30-50ns |
| 中断确认 | 软件 | 读 GIC IAR 获取中断号 | ~20-50ns |
| ISR 处理 | 软件 | 调用对应中断处理函数 | 应用相关 |
| 结束中断 | 软件 | 写 GIC EOIR | ~20-50ns |
| 恢复返回 | 软件 | LDP + ERET | ~30-50ns |

**总中断延迟（不含 ISR）≈ 100-300ns**

## GIC 的角色

```
         ┌───────────┐
  外设1 ─→│           │
  外设2 ─→│  GIC      │──→ CPU0 IRQ
  定时器 ─→│  Distributor│──→ CPU1 IRQ
  UART  ─→│           │──→ CPU2 IRQ
         └───────────┘

GIC 负责：
  1. 接收所有中断源
  2. 优先级仲裁（高优先级先送）
  3. 路由到目标 CPU（亲和性）
  4. 中断确认（IAR）
  5. 中断结束（EOIR）
```

### GIC 关键寄存器

| 寄存器 | 作用 | GICv2 | GICv3 |
|--------|------|-------|-------|
| IAR | 中断确认（读→获取中断号） | GICC_IAR (MMIO) | ICC_IAR1_EL1 (系统寄存器) |
| EOIR | 结束中断 | GICC_EOIR (MMIO) | ICC_EOIR1_EL1 (系统寄存器) |
| PMR | 优先级掩码 | GICC_PMR | ICC_PMR_EL1 |
| CTLR | 使能 GIC | GICD_CTLR | ICC_CTLR_EL1 + GICD_CTLR |

## 中断号

| 中断号范围 | 类型 | 示例 |
|-----------|------|------|
| 0-15 | SGI（软件生成中断） | CPU 间通信（IPI） |
| 16-31 | PPI（私有外设中断） | 定时器（30）、CPU 性能监控 |
| 32-1019 | SPI（共享外设中断） | UART、GPIO、网卡 |
| 1020-1023 | 特殊 | "无中断"占位 |

## HFT 关联

这个完整流程的每一步都贡献延迟。从硬件产生中断到 ISR 开始执行，总延迟 = GIC 仲裁延迟（~50-100ns）+ CPU 响应延迟（当前指令完成，可能 1-10ns）+ 硬件保存（~10ns）+ 向量跳转（~5ns）+ 软件保存（~30-50ns）。总计约 100-200ns。HFT 系统如果用中断接收网卡数据，这个延迟是基础开销，加上 ISR 实际处理时间才是完整中断延迟。**HFT 常用轮询替代中断**来避免 GIC 仲裁和保存/恢复开销。

## 自测题

1. **CPU 什么时候响应 IRQ 中断？是立即响应吗？**
<details><summary>答案</summary>
**不是立即响应**。CPU 在**当前指令执行完后**才响应 IRQ（如果 PSTATE.I 位为 0）。如果当前指令执行时间较长（如 LDP 多个寄存器），中断响应会有额外延迟。某些不可中断的指令序列（如 cache 维护操作）也会延迟响应。
</details>

2. **GIC EOIR 的作用是什么？如果不写会怎样？**
<details><summary>答案</summary>
EOIR（End of Interrupt Register）通知 GIC 中断处理完毕。不写 EOIR → GIC 认为该中断**仍在处理中**，同优先级的中断不再送到 CPU，中断系统实质上卡死。必须在 ISR 末尾写 `EOIR = irq`（用读 IAR 时获取的中断号）。
</details>

3. **从 EL0 用户态发生 IRQ 时，硬件跳到 VBAR_EL1 的哪个偏移？**
<details><summary>答案</summary>
跳到 **VBAR_EL1 + 0x480**（低 EL → 当前 EL AArch64，IRQ）。因为中断来自 EL0（低 EL）→ EL1（当前 EL），AArch64 模式，异常类型是 IRQ。偏移 = 0x400（来源场景）+ 0x080（IRQ）= 0x480。
</details>

4. **SGI、PPI、SPI 三种中断有什么区别？**
<details><summary>答案</summary>
SGI（Software Generated Interrupt，0-15）：软件写 GICD_SGIR 生成，用于 CPU 间通信（IPI）。PPI（Private Peripheral Interrupt，16-31）：每个 CPU 私有，如通用定时器（通常 INT 30），不共享。SPI（Shared Peripheral Interrupt，32-1019）：外设共享，如 UART/GPIO/网卡，可路由到任意 CPU。
</details>

5. **GICv3 用系统寄存器（ICC_IAR1_EL1）确认中断比 GICv2 的 MMIO（GICC_IAR）有什么优势？**
<details><summary>答案</summary>
系统寄存器访问（MRS/MSR）是 CPU 内部操作，不需要总线访问 → 延迟更低且确定。MMIO 访问需要经过总线（AXI/APB）往返到 GIC → 延迟更高且不确定（总线仲裁）。在 HFT 场景中，GICv3 的系统寄存器模式每次中断确认省几十纳秒。
</details>

## 参考与延伸

- [§12.2 中断屏蔽](02-daif.md) — PSTATE.I 位控制是否响应 IRQ
- [§12.4 中断现场保存](04-context-save.md) — 软件保存 X0-X30 的详细代码
- [Ch13 GIC-V2](../../chapter-13-gic-v2/notes/section-0-本章完整概述.md) — GIC 仲裁和 EOIR 的详细原理
