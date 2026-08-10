# 7.4 栈对齐陷阱

> 来源：§7.4 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

AArch64 要求 SP 16 字节对齐，不对齐会触发 SP 对齐异常。

## 核心要点

AAPCS64 规定：
- SP 必须 **16 字节对齐**（SP & 0xF == 0）
- STP/LDP 操作两个 64 位寄存器 = 16 字节
- 不对齐的 SP 访问 → 触发 SP 对齐异常（同步异常）

```asm
; 正确：STP 天然 16 字节对齐
stp x29, x30, [sp, #-16]!   ; sp -= 16（保持对齐）

; 错误：手动破坏对齐
sub sp, sp, #8               ; ❌ sp 变成 8 字节对齐
str x0, [sp]                 ; 可能触发异常
```

SCTLR_EL1 的 SA 位控制 SP 对齐检查：
- SA=1 → EL0 检查 SP 对齐
- SA0=1 → EL1+ 检查 SP 对齐

## HFT 关联

栈对齐影响性能和正确性：
- 16 字节对齐保证 STP/LDP 单次访问完成 → 不对齐可能拆成两次访存
- 某些 SIMD 指令要求 16/32 字节对齐 → 栈不对齐会导致异常
- HFT 中用 SIMD 加速时需确保栈对齐（编译器自动保证）
- 内联汇编中手动操作 SP 需特别注意 16 字节对齐

## 自测题

1. `sub sp, sp, #8` 后执行 `str x0, [sp]` 会发生什么？
<detail><summary>答案</summary>
如果 SCTLR 的 SA/SA0 位开启（默认），SP 不是 16 字节对齐 → STR 触发 SP 对齐异常（同步异常，EC=0x26）。即使 SA 关闭，不对齐的访问也可能性能下降或行为未定义。
</details>

2. 函数栈帧为什么通常分配 16 的倍数字节？
<detail><summary>答案</summary>
1. AAPCS64 要求 SP 16 字节对齐
2. STP/LDP 操作 16 字节，对齐时单次访存
3. 编译器自动按 16 字节倍数分配栈帧
4. 手动分配非 16 倍数会破坏对齐 → 后续 STP/LDP 异常
</details>

3. 如何在 GDB 中检查 SP 是否 16 字节对齐？
<detail><summary>答案</summary>
```gdb
p/x $sp
# 检查低 4 位是否为 0
# 例如 $sp = 0xffff8000a000 → 对齐
#      $sp = 0xffff8000a008 → 不对齐
```
也可以 `p/x $sp & 0xf`，结果为 0 则对齐。
</details>

## 参考与延伸

- 原书 §7.4
- [3.4 STP/LDP](../../chapter-03-a64-load-store/notes/section-0-本章完整概述.md)
- [Ch21 调用约定与栈帧](../../chapter-21-os-topics/notes/section-0-本章完整概述.md)
