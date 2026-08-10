# 8.7 易错点清单

> 来源：§8.7 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

GNU as 汇编器和 C↔汇编互调中常见的错误清单。

## 六大易错点

### 1. 标号不顶格

```asm
; BUG：标号缩进了
    my_label:          ; ❌ 被当作指令
    mov x0, #1

; 正确：标号顶格
my_label:               ; ✓
    mov x0, #1
```

### 2. .global 缺失

```asm
; BUG：汇编函数没有 .global 声明
my_func:                ; ❌ 链接器找不到
    ret

; 正确：加 .global
.global my_func          ; ✓
my_func:
    ret
```

### 3. .align 参数在不同架构含义不同

```asm
; AArch64: .align 4 → 2^4 = 16 字节对齐
; x86:     .align 4 → 4 字节对齐

; 跨架构安全：用 .balign
.balign 16               ; ✓ 所有架构都是 16 字节
```

### 4. callee-saved 寄存器忘记保存

```asm
; BUG：用 X19 不保存
func:
    mov x19, x0          ; ❌ 覆盖调用者的 X19
    bl helper             ; 如果 helper 也用 X19...
    add x0, x19, #1
    ret                   ; 调用者的 X19 被破坏

; 正确：保存 X19
func:
    stp x29, x30, [sp, #-16]!
    str x19, [sp, #-8]!
    mov x19, x0
    bl helper
    add x0, x19, #1
    ldr x19, [sp], #8    ; 恢复
    ldp x29, x30, [sp], #16
    ret
```

### 5. BSS 段写非零初始值

```asm
; BUG：BSS 变量写非零值
.section .bss
counter:
    .quad 42              ; ❌ BSS 应全零，写 42 行为未定义

; 正确：需要非零初始值用 .data
.section .data
counter:
    .quad 42              ; ✓ .data 可以有非零初始值

; .bss 只用于零初始化
.section .bss
buffer:
    .space 256            ; ✓ 全零，加载时零填充
```

### 6. SP 不 16 字节对齐

```asm
; BUG：分配非 16 倍数栈空间
func:
    sub sp, sp, #8        ; ❌ 破坏 16 字节对齐
    str x0, [sp]          ; ❌ 可能触发 SP 对齐异常
    add sp, sp, #8
    ret

; 正确：分配 16 的倍数
func:
    sub sp, sp, #16       ; ✓ 16 的倍数
    str x0, [sp]
    add sp, sp, #16
    ret
```

## 易错点速查表

| 编号 | 易错点 | 正确做法 | 影响 |
|------|--------|----------|------|
| 1 | 标号不顶格 | 标号顶格 | 汇编报错 |
| 2 | .global 缺失 | 声明 .global | 链接报错 |
| 3 | .align 跨架构 | 用 .balign | 对齐错误 |
| 4 | callee-saved 不保存 | 保存 X19-X28 | 数据破坏 |
| 5 | BSS 写非零值 | BSS 只放零初始化 | 行为未定义 |
| 6 | SP 不对齐 | 始终 16 的倍数 | SP 异常 |

## 自测题

1. 以下代码为什么链接报错 `undefined reference to my_func`？
```asm
my_func:
    ret
```
<details><summary>答案</summary>
缺少 `.global my_func` 声明。没有 `.global`，符号只在当前文件内可见。链接器找不到定义。
</details>

2. `.align 3` 在 AArch64 和 x86 上分别对齐多少字节？
<details><summary>答案</summary>
AArch64: 2^3 = 8 字节。x86: 3 字节。跨架构代码需用 `.balign`（统一为字节数）。
</details>

3. 汇编函数中使用 X19 但没保存，会有什么后果？
<details><summary>答案</summary>
X19 是 callee-saved。调用者期望函数返回后 X19 不变。修改了不保存恢复 → 调用者数据被破坏 → 难以调试的 bug。
</details>

4. 以下代码有什么问题？
```asm
.section .bss
counter:
    .quad 42
```
<details><summary>答案</summary>
BSS 段的变量应初始化为零。在 .bss 中写 `.quad 42` 行为未定义——链接器可能忽略这个初始值（视为0），也可能报错。需要非零初始值应在 .data 段定义：
```asm
.section .data
counter:
    .quad 42
```
</details>

5. 以下函数有两个错误，找出它们。
```asm
.global func
func:
    sub sp, sp, #8
    mov x19, x0
    bl helper
    mov x0, x19
    add sp, sp, #8
    ret
```
<details><summary>答案</summary>
错误1：`SUB SP, SP, #8` 破坏了 16 字节对齐。应改为 `SUB SP, SP, #16`。
错误2：`MOV X19, X0` 修改了 callee-saved 寄存器 X19 但没有保存原值。调用者返回后发现 X19 被破坏。应在入口保存 X19，返回前恢复。

修复：
```asm
.global func
func:
    stp x29, x30, [sp, #-16]!   ; 保存 FP+LR（16字节对齐）
    str x19, [sp, #-8]!          ; 额外保存 X19... 但这又破坏对齐
    ; 更好的做法：
    sub sp, sp, #16              ; 额外 16 字节空间
    str x19, [sp]                ; 保存 X19
    mov x19, x0
    bl helper
    mov x0, x19
    ldr x19, [sp]                ; 恢复 X19
    add sp, sp, #16
    ldp x29, x30, [sp], #16     ; 恢复 FP+LR
    ret
```
</details>

## 参考与延伸

- 原书 §8.7
- [8.1 基本语法](01-as-syntax.md)
- [8.3 段](03-sections.md)
- [8.5 C↔汇编互调](05-c-asm-interop.md)
