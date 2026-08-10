# §12.7 易错点清单

> **来源：** [Ch12 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

中断处理中的 7 个常见错误：忘写 EOIR、中断中没重设 TVAL、ISR 用浮点、中断嵌套处理不当、Pi5 用 GICv2 代码、栈空间不足、中断号配置错误。

## 7 大易错点

### 1. 忘写 EOIR

**后果**：GIC 不再发后续中断 → 中断系统停摆。

**原因**：GIC 用 EOIR 追踪中断是否处理完。不写 EOIR → GIC 认为中断仍在处理中 → 同优先级中断不再送到 CPU。

```c
/* ✗ 忘写 EOIR */
void irq_handler(void) {
    u32 irq = gic_read_iar();
    do_something(irq);
    // 忘记 gic_write_eoir(irq)
}

/* ✓ 正确 */
void irq_handler(void) {
    u32 irq = gic_read_iar();
    do_something(irq);
    gic_write_eoir(irq);     // 必须写
}
```

### 2. 中断中没重设 TVAL

**后果**：定时器只触发一次。

**原因**：TVAL 是倒计值，减到 0 触发中断后不自动重载。

```c
/* ✗ 忘记重设 */
void timer_handler(void) {
    // 处理定时器中断
    do_timer_work();
    // 忘记 msr CNTP_TVAL_EL0
}

/* ✓ 重设 TVAL */
void timer_handler(void) {
    do_timer_work();
    asm volatile("msr CNTP_TVAL_EL0, %0" :: "r"(period));
}
```

### 3. ISR 用了浮点

**后果**：被中断代码的浮点寄存器（V0-V31）被破坏。

**原因**：异常入口默认只保存 X0-X30，不保存 V0-V31。ISR 中的浮点运算会覆盖。

```c
/* ✗ ISR 中用浮点 */
void irq_handler(void) {
    float x = 3.14f;      // 用了 V0-V31 中的某个
    float y = x * 2.0f;    // 破坏被中断代码的浮点状态
}

/* ✓ 不用浮点，或额外保存 V0-V31 */
void irq_handler(void) {
    // 只用整数运算
    int result = 42 * 2;   // ✓ 不碰浮点寄存器
}
```

### 4. 中断嵌套没正确处理

**后果**：栈溢出、死锁、优先级反转。

**原因**：异常入口硬件自动关 IRQ。要嵌套需手动 `DAIFClr #0x2`，但风险大。

```asm
/* 危险的嵌套 */
irq_handler:
    sub sp, sp, #272
    stp x0, x1, [sp, #0]
    msr DAIFClr, #0x2       ; 开 IRQ → 允许嵌套！
    bl do_irq                ; 如果 do_irq 中又来 IRQ → 递归
    msr DAIFSet, #0x2       ; 重新关
    ldp x0, x1, [sp, #0]
    add sp, sp, #272
    eret
```

### 5. Pi5 用 GICv2 代码

**后果**：中断完全不工作。

**原因**：GICv2 和 GICv3 的寄存器、访问方式完全不同。

| 操作 | GICv2 (Pi4) | GICv3 (Pi5) |
|------|------------|------------|
| 确认 | MMIO `ldr w0, [gicc, #IAR]` | `mrs x0, ICC_IAR1_EL1` |
| 结束 | MMIO `str w0, [gicc, #EOIR]` | `msr ICC_EOIR1_EL1, x0` |

### 6. 栈空间不足

**后果**：栈溢出 → 覆盖数据 → crash。

**原因**：中断保存需 272 字节，如果嵌套则 ×N。

```ld
/* ✗ 栈太小 */
_stack = . - 1024;  /* 1KB → 一次中断 272B 还够，两次就溢出 */

/* ✓ 栈足够大 */
_stack = . - 16384;  /* 16KB → 足够多次嵌套 */
```

### 7. 中断号配置错误

**后果**：中断不触发或触发到错误的 ISR。

**原因**：定时器中断号是 PPI #30，如果配置为错误的 IRQ 号。

| 中断 | 正确号 | 常见错误 |
|------|--------|---------|
| 通用定时器 | PPI #30 | 配成 SPI 30（不存在的共享中断） |
| UART | SPI #? (查手册) | 和其他设备冲突 |
| SGI | 0-15 | 配成 >15 |

## 易错点速查表

| # | 易错点 | 后果 | 一句话修复 |
|---|--------|------|-----------|
| 1 | 忘写 EOIR | 中断系统卡死 | ISR 末尾写 EOIR |
| 2 | 没重设 TVAL | 只触发一次 | ISR 重写 CNTP_TVAL |
| 3 | ISR 用浮点 | 浮点状态破坏 | 只用整数或额外保存 |
| 4 | 嵌套不当 | 栈溢出/死锁 | 慎重开嵌套 |
| 5 | Pi5 用 GICv2 | 中断不工作 | 改用 GICv3 接口 |
| 6 | 栈空间不足 | 栈溢出 | 16KB+ 栈空间 |
| 7 | 中断号错误 | 中断不触发 | 查手册确认 IRQ 号 |

## 调试技巧

| 症状 | 检查方向 |
|------|---------|
| 中断只触发一次 | 检查 EOIR、TVAL |
| 中断完全不触发 | 检查 GIC 使能、中断号、CNTP_CTL |
| 中断后浮点错误 | 检查 ISR 是否用了浮点 |
| 中断后系统卡死 | 检查是否嵌套导致栈溢出 |
| Pi5 中断不工作 | 确认 GIC 版本，检查 GICv3 初始化 |
| 中断间隔不对 | 检查 CNTFRQ 和 TVAL 值 |

## HFT 关联

HFT 系统中，中断错误通常是致命的。忘写 EOIR 会导致整个中断系统停摆，交易系统无法接收任何中断驱动的事件。ISR 中使用浮点是隐蔽的 bug——异常入口默认不保存 V0-V31 浮点寄存器，ISR 中的浮点运算会破坏被中断代码的浮点状态。建议 ISR 只用整数运算，浮点计算留给非中断上下文。

## 自测题

1. **中断处理完后忘记写 EOIR 会怎样？**
<details><summary>答案</summary>
GIC 认为该中断**仍在处理中**，同优先级的中断不再送到 CPU。实质上中断系统**卡死**——后续中断永远无法到达。修复：ISR 末尾必须写 `GICC->EOIR = irq`（GICv2）或 `msr ICC_EOIR1_EL1, x0`（GICv3），用读 IAR 时获取的中断号。
</details>

2. **为什么 ISR 中不应该使用浮点运算？**
<details><summary>答案</summary>
异常入口的保存代码只保存 **X0-X30 通用寄存器**，不保存 **V0-V31 浮点/NEON 寄存器**。如果 ISR 中使用浮点运算，会覆盖被中断代码的浮点寄存器值，返回后浮点结果错误。如果必须用浮点，需在保存代码中额外保存 V0-V31（但这会增加 ~60 条访存指令，显著增加中断延迟）。
</details>

3. **默认情况下中断能嵌套吗？如何实现中断嵌套？**
<details><summary>答案</summary>
**默认不能嵌套**——进入异常时硬件自动设 PSTATE.I=1（关 IRQ）。要实现嵌套：在 ISR 入口保存完现场后，执行 `msr DAIFClr, #0x2` 手动开中断。但要注意：只有更高优先级的中断才能打断当前 ISR（GIC 优先级控制），且栈空间要足够（每层 272 字节）。
</details>

4. **Pi5 上中断完全不工作，最可能的原因是什么？**
<details><summary>答案</summary>
最可能是**误用 GICv2 代码**。Pi5 用 GICv3（GIC-600），寄存器访问方式完全不同——GICv2 用 MMIO 读写，GICv3 用系统寄存器（ICC_*_EL1）。需要改用 GICv3 的初始化流程和中断确认/结束方式。
</details>

5. **中断处理中栈空间不足会导致什么问题？如何预防？**
<details><summary>答案</summary>
栈溢出 → STP 写到栈外 → 覆盖其他数据 → crash 或行为异常。预防：（1）链接脚本中分配足够大的栈（16KB+）（2）限制中断嵌套深度（3）ISR 中不要使用大的局部数组或递归调用（4）用 ASSERT 检查栈溢出（内核 `CONFIG_DEBUG_STACKOVERFLOW`）。
</details>

## 参考与延伸

- [§12.2 中断屏蔽](02-daif.md) — DAIF 控制中断嵌套
- [§12.3 通用定时器中断](03-timer-interrupt.md) — TVAL 重设和 EOIR 写入
- [§12.5 中断控制器演进](05-irq-controller.md) — GICv2 vs GICv3 的差异
