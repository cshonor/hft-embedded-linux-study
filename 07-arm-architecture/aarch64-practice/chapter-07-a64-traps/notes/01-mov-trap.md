# 7.1 大立即数 MOV 陷阱

> 来源：§7.1 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

MOV 指令无法加载任意 64 位立即数，这是 AArch64 最常见的工程陷阱。

## 核心要点

MOV 限制：
- 只能加载 16 位立即数
- 可配合 `lsl #16/32/48` 移位
- 需要多条 MOVZ/MOVK 组合大常量

```asm
; 正确：加载 0x12345678
movz x0, #0x5678
movk x0, #0x1234, lsl #16

; 错误：mov x0, #0x12345678 → 汇编报错！

; 或者用伪指令（底层生成文字池）
ldr x0, =0x12345678
```

常见错误：
- 在内联汇编中直接 `mov x0, #0x12345678` → 编译报错
- 误以为 MOV 无限制 → 调试困难

## HFT 关联

大常量加载的性能影响：
- MOVZ+MOVK 两条指令 → 2 cycle，无内存访问
- LDR =伪指令 → 1 条指令但需访问文字池 → 可能 cache miss（~12+ cycles）
- HFT 热路径优先用 MOVZ/MOVK 组合（可预测延迟）
- 编译器自动选择最优方式，但内联汇编需手动处理

## 自测题

1. 以下指令哪些合法？
```asm
mov x0, #0x1000
mov x0, #0x10000
mov x0, #0x10001
```
<details><summary>答案</summary>
- `mov x0, #0x1000` 合法（16位内）
- `mov x0, #0x10000` 合法（=1<<16，可移位表示）
- `mov x0, #0x10001` **不合法**（不能单条 MOV 表示，需 MOVZ+MOVK）
</details>

2. 用 MOVZ/MOVK 加载 0xDEADBEEF 到 x0。
<details><summary>答案</summary>
```asm
movz x0, #0xBEEF
movk x0, #0xDEAD, lsl #16
```
</details>

3. MOVZ+MOVK 和 LDR =哪个更适合 HFT 热路径？
<detail><summary>答案</summary>
MOVZ+MOVK 更适合。两条指令 2 cycle，无内存依赖，延迟可预测。LDR =需要访问文字池（内存），可能 cache miss 导致 ~12+ cycle 不可预测延迟。但 LDR =只需 1 条指令，代码更紧凑，适合非热路径。
</details>

## 参考与延伸

- 原书 §7.1
- [3.5 LDR 伪指令](../../chapter-03-a64-load-store/notes/section-0-本章完整概述.md)
- ARM ARM §C3.4
