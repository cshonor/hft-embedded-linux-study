# §11.8 易错点清单

> **来源：** [Ch11 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

异常处理中最常见的 7 个错误：向量表没对齐、忘记保存通用寄存器、ERET 前 SP 没恢复、SP0/SPx 选错、忘读 ESR/FAR、EL 降级配置遗漏、DAIF 未正确处理。

## 7 大易错点

### 1. 向量表没对齐

**后果**：异常发生时跳转到错误地址 → 立即死机。

**原因**：VBAR 寄存器要求低 11 位为 0（2048 字节对齐）。不对齐则 `VBAR + offset` 计算出错误地址。

```asm
/* ✗ 错误：没有对齐 */
vector_table:
    b sync_handler

/* ✓ 正确：2048 字节对齐 */
.align 11
vector_table:
    .align 7
    b sync_handler
```

### 2. 忘记保存通用寄存器

**后果**：ISR 修改 X0-X30 后，ERET 回到被中断代码 → 寄存器值被破坏。

**原因**：硬件只保存 ELR/SPSR，X0-X30 需软件保存。

```asm
/* ✗ 错误：没保存寄存器直接调用 C */
irq_handler:
    bl do_irq          /* do_irq 可能修改 X0-X30 */
    eret               /* 返回后寄存器被破坏 */

/* ✓ 正确：保存后再调用 */
irq_handler:
    sub sp, sp, #272
    stp x0, x1, [sp, #0]
    /* ... 保存全部 */
    bl do_irq
    ldp x0, x1, [sp, #0]
    /* ... 恢复全部 */
    add sp, sp, #272
    eret
```

### 3. ERET 前 SP 没恢复

**后果**：ERET 后 SP 指向错误地址 → 后续函数调用栈溢出或覆盖数据。

**原因**：ERET 恢复 PC 和 PSTATE 但不恢复 SP。如果 `sub sp, sp, #272` 后忘记 `add sp, sp, #272`。

```asm
/* ✗ 错误：分配栈空间后没恢复 */
    sub sp, sp, #272
    stp x0, x1, [sp, #0]
    bl do_irq
    ldp x0, x1, [sp, #0]
    eret              /* SP 仍然偏移了 272！ */

/* ✓ 正确：ERET 前恢复 SP */
    sub sp, sp, #272
    stp x0, x1, [sp, #0]
    bl do_irq
    ldp x0, x1, [sp, #0]
    add sp, sp, #272  /* 恢复 SP */
    eret
```

### 4. SP0/SPx 选错

**后果**：使用错误的栈指针 → 栈混乱、覆盖数据。

**原因**：向量表 16 项分 4 组，前 4 项用 SP_EL0，后 12 项用 SP_ELx。选错组导致用错误的 SP。

| 场景 | 正确的表项 | SP |
|------|----------|-----|
| 内核态 IRQ | offset 0x280 (SPx) | SP_EL1 |
| 用户态 SVC | offset 0x400 (低EL A64) | 切到 SP_EL1 |
| 内核态同步异常 | offset 0x200 (SPx) | SP_EL1 |

### 5. 忘读 ESR/FAR

**后果**：无法诊断同步异常原因，只能盲猜。

**原因**：同步异常原因在 ESR_ELx，故障地址在 FAR_ELx。不读无法定位。

```c
/* ✗ 错误：异常处理只打印 "error" */
void sync_handler(void) {
    printf("Exception occurred!\n");
    /* 然后呢？ */
}

/* ✓ 正确：读 ESR/FAR/ELR 精确定位 */
void sync_handler(void) {
    u64 esr = read_sysreg(ESR_EL1);
    u64 far = read_sysreg(FAR_EL1);
    u64 elr = read_sysreg(ELR_EL1);
    u32 ec = esr >> 26;
    printf("EC=0x%x FAR=0x%llx ELR=0x%llx\n", ec, far, elr);
}
```

### 6. EL 降级配置遗漏

**后果**：降级后某些操作被 trap 到更高 EL → 不可预期的延迟或异常。

**原因**：从 EL2 降级时没正确配置 HCR_EL2（如没禁用虚拟化陷阱）。

```asm
/* ✗ 不完整：只设 SPSR/ELR 没配 HCR_EL2 */
from_el2:
    mov x0, #0x3c5
    msr SPSR_EL2, x0
    adr x0, in_el1
    msr ELR_EL2, x0
    eret              /* EL1 可能仍被 trap */

/* ✓ 完整：配 HCR_EL2.RW=1 禁用 trap */
from_el2:
    mov x0, #(1 << 31)     /* HCR_EL2.RW=1 → EL1 AArch64 */
    msr HCR_EL2, x0
    mov x0, #0x3c5
    msr SPSR_EL2, x0
    adr x0, in_el1
    msr ELR_EL2, x0
    eret
```

### 7. DAIF 中断屏蔽未正确处理

**后果**：中断被意外屏蔽/开启 → 中断丢失或中断嵌套过深。

**原因**：异常进入时硬件自动屏蔽 DAIF，但 ERET 恢复 SPSR 时会恢复 DAIF 状态。如果在 ISR 中手动修改 DAIF 需注意恢复。

```asm
/* 异常进入时硬件自动屏蔽 DAIF */
/* ERET 时从 SPSR 恢复 DAIF → 自动恢复 */

/* 但如果在 ISR 中手动开关中断需注意 */
irq_handler:
    /* DAIF 已被硬件屏蔽 */
    msr daifclr, #2         /* 手动开 IRQ → 允许嵌套！危险 */
    bl do_irq               /* 如果 do_irq 中又来 IRQ → 嵌套 */
    /* ... */
    eret                    /* 从 SPSR 恢复 DAIF */
```

## 易错点速查表

| # | 易错点 | 后果 | 一句话修复 |
|---|--------|------|-----------|
| 1 | 向量表没对齐 | 跳到错误地址 | `.align 11` |
| 2 | 忘记保存 X0-X30 | 寄存器被覆盖 | STP 压栈/LDP 恢复 |
| 3 | ERET 前 SP 没恢复 | 栈错位 | `add sp, sp, #272` |
| 4 | SP0/SPx 选错 | 栈混乱 | 确认来源场景 |
| 5 | 忘读 ESR/FAR | 无法诊断 | 同步异常必读 ESR |
| 6 | EL 降级配置遗漏 | 操作被 trap | 配 HCR_EL2 |
| 7 | DAIF 处理错误 | 中断丢失/嵌套 | 注意 SPSR 恢复 |

## 调试技巧

| 症状 | 检查方向 |
|------|---------|
| 异常后死机 | VBAR 是否正确设置、向量表是否 2048B 对齐 |
| 寄存器值莫名其妙被改 | 异常入口是否保存了 X0-X30 |
| ERET 后崩溃 | SP 是否恢复、SPSR 是否正确 |
| 无法定位异常原因 | 读 ESR_ELx 的 EC + FAR_ELx |
| 降级后操作被 trap | HCR_EL2 是否正确配置 |
| QEMU 上跑正常，Pi 上死 | EL 启动级别不同（QEMU EL1 vs Pi EL2/3） |
| 中断偶尔丢失 | DAIF 是否意外屏蔽了 IRQ |

## HFT 关联

HFT 系统中异常处理错误通常是致命的——一次未处理的页错误就会导致整个交易系统停机。建议在开发阶段开启同步异常的详细日志（打印 ESR/FAR/ELR），上线前移除。向量表对齐问题是新手最常见的坑，用 QEMU 调试时可以通过 `-d int` 选项查看异常跳转地址是否正确。

## 自测题

1. **向量表起始地址必须满足什么对齐要求？为什么？**
<details><summary>答案</summary>
必须 **2048 字节对齐**（`.align 11`）。因为 VBAR 寄存器的低 11 位必须为 0，硬件用 `VBAR + offset`（offset 最大 0x780）计算跳转地址。不对齐会导致跳转到错误地址 → 立即死机。
</details>

2. **异常处理中 ERET 之前必须确保什么？如果不做会怎样？**
<details><summary>答案</summary>
必须确保 **SP 已恢复**（`add sp, sp, #272`）。ERET 只恢复 PC 和 PSTATE，不恢复 SP。如果 SP 没恢复，ERET 后 SP 偏移了 272 字节，后续函数调用会在错误栈位置压栈 → 覆盖数据或栈溢出。
</details>

3. **同步异常发生后应该读哪些寄存器来诊断原因？**
<details><summary>答案</summary>
读 **ESR_ELx**（异常综合征，EC 字段在 bit[31:26] 判断异常类型）和 **FAR_ELx**（故障虚拟地址，对于数据中止有效）。还应该读 **ELR_ELx**（异常返回地址，定位触发指令）。三个寄存器一起读可以精确定位异常原因和位置。
</details>

4. **从 EL2 降级到 EL1 时遗漏配置 HCR_EL2 会导致什么问题？**
<details><summary>答案</summary>
HCR_EL2 控制是否将 EL1 的操作 trap 到 EL2。如果不配（默认可能有陷阱），EL1 的 MMU 配置、系统寄存器访问、中断处理等可能被 trap → 额外异常 → 死机或延迟。最关键的是 HCR_EL2.RW 位必须设为 1，否则 EL1 用 AArch32 而非 AArch64。
</details>

5. **异常处理中手动 `msr daifclr, #2` 开中断有什么风险？**
<details><summary>答案</summary>
开 IRQ 允许中断嵌套——在处理一个 IRQ 时又来一个 IRQ → 递归进入异常处理 → 栈快速消耗 → 栈溢出。通常 ISR 中不开中断（保持 DAIF 屏蔽），等 ERET 恢复 SPSR 时自动恢复。如果确实需要嵌套，需仔细设计栈大小和嵌套深度限制。
</details>

## 参考与延伸

- [§11.3 异常向量表](03-vector-table.md) — 陷阱 1 的详解
- [§11.4 硬件保存+软件保存](04-hw-sw-save.md) — 陷阱 2/3 的详解
- [§11.5 异常综合征](05-esr.md) — 陷阱 5 的详解
- [§11.6 EL2→EL1](06-el2-to-el1.md) — 陷阱 6 的详解
