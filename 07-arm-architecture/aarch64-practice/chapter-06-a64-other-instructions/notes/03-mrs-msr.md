# 6.3 MRS / MSR 系统寄存器读写

> 来源：§6.3 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

MRS/MSR 指令读写系统寄存器（TTBR、SCTLR、VBAR 等），只能在 EL1+ 执行。

## 核心要点

```asm
; 读系统寄存器到通用寄存器
mrs x0, SCTLR_EL1      ; 读系统控制寄存器

; 写通用寄存器到系统寄存器
msr SCTLR_EL1, x0      ; 写系统控制寄存器
```

- MRS：System → General（读系统寄存器）
- MSR：General → System（写系统寄存器）
- 只能在 EL1+ 执行（EL0 执行会触发非法异常）
- 常用系统寄存器：TTBR/TCR/SCTLR(MMU)、VBAR(向量表)、ESR/FAR(异常)、CurrentEL

## HFT 关联

系统寄存器控制 CPU 行为，影响性能：
- SCTLR 的 C 位控制 D-cache → 关闭 cache 性能暴跌
- TCR 的 cache 特性影响 MMU walk 延迟
- 读写系统寄存器有特殊延迟（~10+ cycles，比 L1 慢）
- 频繁读系统寄存器（如读 cntvct_el0 时间戳）需考虑其延迟

## 自测题

1. 在 EL0 执行 `msr SCTLR_EL1, x0` 会发生什么？
<detail><summary>答案</summary>
触发同步异常（非法指令使用）。系统寄存器只能在 EL1+ 访问，EL0 没有权限。异常向量表中同步异常表项会处理这个错误，通常发送 SIGILL 信号给用户进程。
</details>

2. 如何读取当前异常等级？
<detail><summary>答案</summary>
```asm
mrs x0, CurrentEL
lsr x0, x0, #2    ; EL 在 bit[3:2]
```
CurrentEL 寄存器的 bit[3:2] 存储当前 EL（0-3），右移 2 位得到 EL 值。
</details>

3. MRS/MSR 的延迟为什么比普通寄存器操作高？
<detail><summary>答案</summary>
系统寄存器通常控制硬件状态（MMU/GIC/中断），读写需要同步到硬件，可能涉及 pipeline flush 或特定序列化。延迟 ~10+ cycles，远高于 L1 cache 的 ~4 cycles。
</details>

## 参考与延伸

- 原书 §6.3
- [Ch14 MMU 寄存器](../../chapter-14-memory-management/notes/section-0-本章完整概述.md)
- [Ch11 异常寄存器](../../chapter-11-exception-handling/notes/section-0-本章完整概述.md)
