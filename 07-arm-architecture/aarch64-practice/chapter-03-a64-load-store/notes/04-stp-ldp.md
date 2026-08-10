# 3.4 LDP / STP 栈操作主力

> 来源：§3.4 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

AArch64 没有 PUSH/POP，用 STP/LDP 一次存取两个寄存器，是栈操作的主力。

## 核心要点

```asm
; 压栈：保存 FP(x29) 和 LR(x30)
stp x29, x30, [sp, #-16]!   ; sp -= 16; 存储 x29→[sp], x30→[sp+8]

; 出栈
ldp x29, x30, [sp], #16     ; 加载 [sp]→x29, [sp+8]→x30; sp += 16
```

- 一对 64 位寄存器 = 16 字节
- **AAPCS64** 标准函数入口保存 FP+LR
- 栈向下生长（SP 减小）
- STP/LDP 比两条 STR/LDR 更高效（一条指令完成两个寄存器的存取）

## HFT 关联

STP/LDP 对函数调用开销的影响：
- 每次函数调用至少 STP x29,x30（保存）+ LDP x29,x30（恢复）= 2 条内存指令
- 寄存器保存/恢复是函数调用开销的主要来源 → 热路径函数应减少调用层级
- `-fomit-frame-pointer` 可以省掉 x29 的保存（如果不需要回溯），但 x30(LR) 仍需保存
- 叶子函数（不调用其他函数）可以完全省略栈帧 → 最小化调用开销

## 自测题

1. AArch64 为什么没有 PUSH/POP？
<details><summary>答案</summary>
PUSH/POP 只操作一个寄存器且隐式修改 SP。STP/LDP 一次操作两个寄存器更高效，且显式控制 SP 变化（用前/后变基），设计更灵活统一。
</details>

2. 以下函数入口代码保存了什么？栈帧多大？
```asm
func:
    stp x29, x30, [sp, #-16]!
    mov x29, sp
```
<details><summary>答案</summary>
保存了 FP(x29) 和 LR(x30)。栈帧 16 字节。`mov x29, sp` 设置帧指针指向当前栈顶，便于调试回溯。
</details>

3. 叶子函数（不调用其他函数）可以省略 STP x29,x30 吗？
<details><summary>答案</summary>
可以省略 x30(LR) 的保存（因为不会被 BL 覆盖）。x29(FP) 的保存取决于是否需要帧指针。叶子函数可以完全没有栈帧。
</details>

## 参考与延伸

- 原书 §3.4
- [3.3 寻址模式](03-addressing-modes.md)
- [Ch21 调用约定与栈帧](../../chapter-21-os-topics/notes/section-0-本章完整概述.md)
