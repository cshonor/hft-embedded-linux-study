# 6.3 MRS / MSR 系统寄存器读写

> 来源：§6.3 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

MRS/MSR 指令读写系统寄存器（TTBR、SCTLR、VBAR 等），这些寄存器控制 CPU 的 MMU、缓存、中断等核心行为。只能在 EL1+ 执行。

## 核心要点

### 指令格式

```asm
; MRS：读系统寄存器到通用寄存器（System → General）
MRS Xd, <sysreg>      ; Xd = <sysreg> 的值

; MSR：写通用寄存器到系统寄存器（General → System）
MSR <sysreg>, Xn      ; <sysreg> = Xn 的值

; MSR 也可写立即数（部分系统寄存器支持）
MSR <sysreg>, #imm    ; 仅限少数寄存器（如 PSTATE.DAIF）
```

### 记忆方式

```
MRS = Move from Register (System to general)
     M - R - S：Move Register System → 读系统寄存器

MSR = Move to Register (System) (general to System)
     M - S - R：Move System Register → 写系统寄存器

记忆口诀：
  MRS：S 在最后 → 从 System 读出
  MSR：S 在中间 → 写入 System
```

### 常用系统寄存器

| 寄存器 | 全称 | 用途 | 典型操作 |
|--------|------|------|----------|
| CurrentEL | Current Exception Level | 当前异常等级 | `MRS x0, CurrentEL` |
| DAIF | Debug/Abort/IRQ/FIQ | 中断屏蔽 | `MSR DAIFSet, #0xF` |
| VBAR_EL1 | Vector Base Address | 异常向量表基址 | `MSR VBAR_EL1, x0` |
| SCTLR_EL1 | System Control | MMU/Cache 开关 | `MRS x0, SCTLR_EL1` |
| TTBR0_EL1 | Translation Table Base 0 | 用户空间页表基址 | `MSR TTBR0_EL1, x0` |
| TTBR1_EL1 | Translation Table Base 1 | 内核空间页表基址 | `MSR TTBR1_EL1, x0` |
| TCR_EL1 | Translation Control | 页表控制（ASID/范围） | `MRS x0, TCR_EL1` |
| ESR_EL1 | Exception Syndrome | 异常信息 | `MRS x0, ESR_EL1` |
| ELR_EL1 | Exception Link Register | 异常返回地址 | `MRS x0, ELR_EL1` |
| FAR_EL1 | Fault Address | 异常地址 | `MRS x0, FAR_EL1` |
| CNTVCT_EL0 | Virtual Timer Count | 时间戳计数器 | `MRS x0, CNTVCT_EL0` |

### CurrentEL 寄存器

```asm
; 读取当前异常等级
MRS x0, CurrentEL
LSR x0, x0, #2        ; EL 在 bit[3:2]，右移2位
AND x0, x0, #3        ; 取低2位 → 0/1/2/3

; CurrentEL 格式：
; [3:2] = EL (0-3)
; 其他位保留
; 例：EL1 → CurrentEL = 0b0100 = 0x4
;     EL2 → CurrentEL = 0b1000 = 0x8
```

### DAIF 中断屏蔽

```asm
; DAIF 寄存器控制 4 类中断的屏蔽
; D = Debug exception mask
; A = SError (Abort) mask
; I = IRQ mask
; F = FIQ mask

; 屏蔽所有中断
MSR DAIFSet, #0xF     ; 设置 D/A/I/F 全部屏蔽

; 开启 IRQ 和 FIQ
MSR DAIFClr, #0xC     ; 清除 I/F（bit2=I, bit3=F）

; 在 Linux 内核中常用汇编宏
.macro disable_irq
    msr daifset, #0xf
.endm

.macro enable_irq
    msr daifclr, #0xf
.endm
```

### CNTVCT_EL0 时间戳

```asm
; 读取虚拟时间戳计数器（可用于高精度计时）
MRS x0, CNTVCT_EL0    ; 64位时间戳，频率由 CNTFRQ_EL0 决定

; 计算代码执行时间
MRS x1, CNTVCT_EL0    ; 开始时间
; ... 待测代码 ...
MRS x2, CNTVCT_EL0    ; 结束时间
SUB x3, x2, x1        ; 差值（周期数）

; 转换为纳秒：差值 × 1e9 / CNTFRQ_EL0
MRS x4, CNTFRQ_EL0    ; 频率（如 62.5MHz → 62500000）
; ns = diff * 1e9 / freq
```

### MSR 的序列化效应

```asm
; 写某些系统寄存器后需要 ISB 保证生效
MSR SCTLR_EL1, x0     ; 修改 MMU/Cache 设置
ISB                    ; 必须加 ISB！冲刷流水线确保后续指令用新配置

MSR VBAR_EL1, x0      ; 修改异常向量表地址
ISB                    ; 确保后续异常使用新向量表

; 开启 MMU 的经典序列
MRS x0, SCTLR_EL1
ORR x0, x0, #1        ; set MMU enable bit
MSR SCTLR_EL1, x0
ISB                    ; 关键：不加 ISB 后果不可预测
```

## 权限模型

| 寄存器 | EL0 | EL1 | EL2 | EL3 |
|--------|-----|-----|-----|-----|
| CurrentEL | ✓ | ✓ | ✓ | ✓ |
| CNTVCT_EL0 | ✓ | ✓ | ✓ | ✓ |
| DAIF | ✗ | ✓ | ✓ | ✓ |
| SCTLR_EL1 | ✗ | ✓ | ✓ | ✓ |
| TTBR_EL1 | ✗ | ✓ | ✓ | ✓ |
| VBAR_EL1 | ✗ | ✓ | ✓ | ✓ |
| SCTLR_EL2 | ✗ | ✗ | ✓ | ✓ |

> EL0 执行 MRS/MSR 访问 EL1 寄存器 → 触发同步异常（非法指令使用）。

## 与 C 的对照

```c
// Linux 内核中使用内联汇编读写系统寄存器
// 读 SCTLR_EL1
static inline u64 read_sctlr_el1(void) {
    u64 val;
    asm volatile("mrs %0, sctlr_el1" : "=r"(val));
    return val;
}

// 写 SCTLR_EL1
static inline void write_sctlr_el1(u64 val) {
    asm volatile("msr sctlr_el1, %0" : : "r"(val));
    asm volatile("isb");  // 写系统寄存器后加 ISB
}
```

## 常见错误

1. **EL0 执行 MRS/MSR**：用户态无权限，触发非法异常。
2. **写系统寄存器后忘记 ISB**：流水线中的旧指令可能用旧配置，导致不可预测行为。
3. **混淆 _EL1/_EL2 后缀**：SCTLR_EL1 和 SCTLR_EL2 是不同寄存器，写错了无效。

## HFT 关联

系统寄存器控制 CPU 行为，影响性能：
- SCTLR 的 C 位控制 D-cache → 关闭 cache 性能暴跌
- TCR 的 cache 特性影响 MMU walk 延迟
- 读写系统寄存器有特殊延迟（~10+ cycles，比 L1 慢）
- 频繁读系统寄存器（如读 cntvct_el0 时间戳）需考虑其延迟

```c
// HFT：用 CNTVCT_EL0 做高精度计时
static inline uint64_t rdtsc(void) {
    uint64_t val;
    asm volatile("mrs %0, cntvct_el0" : "=r"(val));
    return val;
}

// 测量函数延迟
uint64_t start = rdtsc();
process_order();
uint64_t end = rdtsc();
uint64_t cycles = end - start;
// 注意：MRS CNTVCT_EL0 本身约 10+ cycles
// 测量短函数时需要减去 MRS 的开销
```

## 自测题

1. 在 EL0 执行 `msr SCTLR_EL1, x0` 会发生什么？
<details><summary>答案</summary>
触发同步异常（非法指令使用）。系统寄存器只能在 EL1+ 访问，EL0 没有权限。异常向量表中同步异常表项会处理这个错误，通常发送 SIGILL 信号给用户进程。
</details>

2. 如何读取当前异常等级？
<details><summary>答案</summary>
```asm
mrs x0, CurrentEL
lsr x0, x0, #2    ; EL 在 bit[3:2]
```
CurrentEL 寄存器的 bit[3:2] 存储当前 EL（0-3），右移 2 位得到 EL 值。
</details>

3. MRS/MSR 的延迟为什么比普通寄存器操作高？
<details><summary>答案</summary>
系统寄存器通常控制硬件状态（MMU/GIC/中断），读写需要同步到硬件，可能涉及 pipeline flush 或特定序列化。延迟 ~10+ cycles，远高于 L1 cache 的 ~4 cycles。
</details>

4. 为什么写 SCTLR_EL1 后必须加 ISB？
<details><summary>答案</summary>
SCTLR_EL1 控制 MMU 和 Cache 的开关。写 SCTLR 后，CPU 流水线中可能还有使用旧配置（如 MMU 关闭状态）取的指令。ISB 冲刷流水线，强制后续指令用新配置重新取指。如果不加 ISB，后续指令可能用错误的地址翻译或缓存配置，导致不可预测行为。
</details>

5. 写出用 CNTVCT_EL0 测量一段代码延迟的完整汇编代码。
<details><summary>答案</summary>
```asm
; 测量 my_func 的执行周期数
MRS x1, CNTVCT_EL0     ; 开始时间戳
BL my_func              ; 待测函数
MRS x2, CNTVCT_EL0     ; 结束时间戳
SUB x3, x2, x1          ; x3 = 周期差值
MRS x4, CNTFRQ_EL0      ; 获取频率
; 如需转纳秒：ns = diff * 1e9 / freq
```
注意：MRS CNTVCT_EL0 本身约 10+ cycles，测量极短代码时需减去这个基准开销。
</details>

## 参考与延伸

- 原书 §6.3
- [Ch14 MMU 寄存器](../../chapter-14-memory-management/notes/section-0-本章完整概述.md)
- [Ch11 异常寄存器](../../chapter-11-exception-handling/notes/section-0-本章完整概述.md)
