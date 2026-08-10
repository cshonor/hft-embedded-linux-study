# 8.4 宏

> 来源：§8.4 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

GNU as 的宏定义和使用，用 `.macro`/`.endm` 减少重复汇编代码。宏在汇编阶段展开，无运行时开销。

## 核心要点

### 基本宏定义

```asm
.macro SAVE_REGS
    stp x29, x30, [sp, #-16]!
    stp x0,  x1,  [sp, #-16]!
    stp x2,  x3,  [sp, #-16]!
.endm

.macro RESTORE_REGS
    ldp x2,  x3,  [sp], #16
    ldp x0,  x1,  [sp], #16
    ldp x29, x30, [sp], #16
.endm

irq_handler:
    SAVE_REGS               ; 展开为上面的 6 条 STP
    ; ... 处理中断 ...
    RESTORE_REGS
    eret
```

### 带参数的宏

```asm
; 带一个参数
.macro PUTCHAR char
    mov w0, #\char          ; \char 是参数引用
    bl  print_char
.endm

    PUTCHAR 'H'             ; → mov w0, #'H'; bl print_char
    PUTCHAR 'i'             ; → mov w0, #'i'; bl print_char

; 带多个参数
.macro REG_SAVE reg1, reg2
    stp \reg1, \reg2, [sp, #-16]!
.endm

    REG_SAVE x19, x20       ; → stp x19, x20, [sp, #-16]!
    REG_SAVE x21, x22       ; → stp x21, x22, [sp, #-16]!
```

### 带默认值的参数

```asm
.macro ALLOC_STACK size=16
    sub sp, sp, #\size
.endm

    ALLOC_STACK             ; → sub sp, sp, #16（默认值）
    ALLOC_STACK 32          ; → sub sp, sp, #32
```

### 宏内的循环

```asm
; 用 .irp 实现循环展开
.macro SAVE_ALL_REGS
    .irp i, 0, 1, 2, 3, 4, 5, 6, 7
        stp x\i, x\n(\i+8), [sp, #-16]!
    .endr
.endm

; 展开为：
;   stp x0, x8, [sp, #-16]!
;   stp x1, x9, [sp, #-16]!
;   stp x2, x10, [sp, #-16]!
;   ... 直到 x7, x15
```

> `\n(\i+8)` 是 GNU as 的算术语法，在宏内计算 `\i + 8` 的值。

### 条件汇编

```asm
.macro SAVE_REGS, count
    .if \count > 0
        stp x0, x1, [sp, #-16]!
    .endif
    .if \count > 2
        stp x2, x3, [sp, #-16]!
    .endif
    .if \count > 4
        stp x4, x5, [sp, #-16]!
    .endif
.endm

    SAVE_REGS 6             ; 保存 x0-x5（3 组 STP）
    SAVE_REGS 2             ; 只保存 x0-x1（1 组 STP）
```

### 宏退出

```asm
.macro FIND_REG reg
    .ifc \reg, x0
        mov x9, #0
        .exitm              ; 提前退出宏
    .endifc
    .ifc \reg, x1
        mov x9, #1
        .exitm
    .endifc
    mov x9, #-1             ; 未找到
.endm
```

### 宏 vs C 内联函数

| 特性 | 汇编宏 | C 内联函数 |
|------|--------|-----------|
| 展开时机 | 汇编阶段 | 编译阶段 |
| 参数类型 | 无类型（文本替换） | 有类型检查 |
| 调试 | GDB 看到展开后代码 | GDB 能映射到源码 |
| 代码体积 | 完全展开 | 编译器可能优化 |
| 适用场景 | 纯汇编文件 | C 中的汇编片段 |

## 与 C 的对照

```c
// C 宏（预处理阶段展开）
#define SAVE_REGS() \
    asm volatile("stp x29, x30, [sp, #-16]!")

// C 内联函数（编译阶段处理）
static inline void save_regs(void) {
    asm volatile("stp x29, x30, [sp, #-16]!");
}

// C 内联函数有类型检查，调试友好
// 汇编宏在纯 .S 文件中使用
```

## 常见错误

1. **忘记 .endm**：宏定义不闭合 → 后续代码全部被当作宏体。
2. **参数引用不加反斜杠**：用 `char` 而非 `\char` → 不被识别为参数。
3. **宏内破坏对齐**：每个 STP 必须 16 字节，宏展开后可能不对齐。

## HFT 关联

- 异常处理中保存/恢复寄存器的代码重复 → 宏消除重复
- 但宏调试困难（GDB 看到的是展开后的代码）
- 现代倾向用 C 内联函数替代汇编宏
- Linux 内核大量使用汇编宏（如 kernel_entry/kernel_exit）

```asm
; Linux 内核中的宏示例（arch/arm64/include/asm/assembler.h）
.macro kernel_entry, el, regsize=64
    .if \regsize == 32
        // 32 位兼容模式处理
    .else
        stp x0, x1, [sp, #16*0]
        stp x2, x3, [sp, #16*1]
        // ... 保存所有寄存器 ...
    .endif
    // 设置当前 EL、保存 PSTATE 等
.endm
```

## 自测题

1. 汇编宏和 C 宏有什么区别？
<details><summary>答案</summary>
展开时机：汇编宏在汇编阶段，C 宏在预处理阶段。参数类型：汇编宏参数无类型，纯文本替换。
</details>

2. 宏参数引用用什么符号？
<details><summary>答案</summary>
反斜杠 `\`。如 `.macro FUNC reg` 中引用参数用 `\reg`。
</details>

3. 以下宏有什么效率问题？
```asm
.macro PUSH reg
    str \reg, [sp, #-8]!
.endm
```
<details><summary>答案</summary>
str 单个寄存器入栈效率不如 stp。多次 PUSH 后 SP 可能不 16 对齐。应改用 stp 一次存两个寄存器。
</details>

4. 写一个宏，根据参数 `count` 保存前 N 对寄存器（x0-x(N*2-1)）。
<details><summary>答案</summary>
```asm
.macro SAVE_N_PAIRS count
    .if \count > 0
        stp x0, x1, [sp, #-16]!
    .endif
    .if \count > 1
        stp x2, x3, [sp, #-16]!
    .endif
    .if \count > 2
        stp x4, x5, [sp, #-16]!
    .endif
    .if \count > 3
        stp x6, x7, [sp, #-16]!
    .endif
.endm
```
条件汇编 `.if` 确保 count=1 时只保存 x0,x1，count=3 时保存 x0-x5。
</details>

5. `.exitm` 和 `.endm` 有什么区别？
<details><summary>答案</summary>
- `.endm`：标记宏定义的结束，必须出现在每个宏的末尾。
- `.exitm`：从宏中提前退出（类似 C 的 return），跳过宏体中后续的指令。常用于条件判断后提前返回。`.exitm` 后宏的展开立即停止，但 `.endm` 仍然需要存在作为宏定义的终结。
</details>

## 参考与延伸

- 原书 §8.4
- [8.5 C↔汇编互调](05-c-asm-interop.md)
- [8.2 伪指令（.if/.endif）](02-directives.md)
- GNU as Manual §7
