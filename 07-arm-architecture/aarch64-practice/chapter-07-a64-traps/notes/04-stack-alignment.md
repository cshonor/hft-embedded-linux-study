# 7.4 栈对齐陷阱

> 来源：§7.4 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

AArch64 要求 SP（栈指针）16 字节对齐，不对齐会触发 SP 对齐异常。这是裸机开发和内联汇编中最常见的崩溃原因之一。

## 核心要点

### AAPCS64 栈对齐规则

```
AAPCS64（ARM Architecture Procedure Call Standard for AArch64）规定：
  SP 必须 16 字节对齐（SP & 0xF == 0）

原因：
  STP/LDP 操作两个 64 位寄存器 = 16 字节
  16 字节对齐保证 STP/LDP 单次总线访问完成
  不对齐 → 可能拆成两次访问 → 性能下降或异常
```

### 对齐检查控制

```
SCTLR_EL1 的相关位：
  SA  (bit 18)  → SP 对齐检查 for EL0
  SA0 (bit 4)   → SP 对齐检查 for EL1+

  SA=1, SA0=1 → 严格检查（Linux 默认开启）
  SA=0, SA0=0 → 不检查（调试时可能关闭，不推荐）

检查失败 → 触发同步异常（SP 对齐异常, EC=0x26）
```

### 正确的栈操作

```asm
; 正确：STP/LDP 天然 16 字节对齐
STP X29, X30, [SP, #-16]!   ; SP -= 16，保存帧指针和返回地址
; ... 函数体 ...
LDP X29, X30, [SP], #16     ; 恢复，SP += 16

; 正确：分配 16 的倍数栈空间
SUB SP, SP, #32              ; 分配 32 字节（16 的倍数）
STR X0, [SP]
STR X1, [SP, #8]
ADD SP, SP, #32              ; 释放

; 正确：大栈帧
SUB SP, SP, #48              ; 48 = 16×3 ✓
STP X19, X20, [SP]
STP X21, X22, [SP, #16]
STP X23, X24, [SP, #32]
ADD SP, SP, #48
```

### 错误的栈操作

```asm
; 错误：分配 8 字节（非 16 倍数）
SUB SP, SP, #8               ; ❌ SP 变成 8 字节对齐
STR X0, [SP]                 ; 可能触发 SP 对齐异常
ADD SP, SP, #8

; 错误：手动破坏对齐
MOV X0, SP
SUB SP, SP, #4               ; ❌ SP 错位
STR W1, [SP]                 ; ❌ 32位 STR 在不对齐的 SP 上

; 错误：内联汇编中操作 SP
// C 代码中嵌入的汇编
asm volatile("sub sp, sp, #8");  // ❌ 破坏对齐！
```

### 变长数组（VLA）的对齐

```asm
; 变长数组分配时编译器自动对齐
// C 代码
void func(int n) {
    int arr[n];  // VLA
    // 编译器生成：
    // SUB SP, SP, #(n*4 + padding) → 保证 16 字节对齐
    // AND SP, SP, #~0xF            → 额外对齐保证
}

; 手动实现 VLA 对齐
; X0 = 需要的字节数
MOV X1, SP
SUB X2, X1, X0            ; 预分配
AND SP, X2, #~0xF         ; 16 字节对齐
; ... 使用栈空间 ...
MOV SP, X1                 ; 恢复
```

### 函数调用与栈对齐

```
AAPCS64 调用约定：
  函数入口时 SP 必须 16 字节对齐
  函数返回时 SP 必须恢复到调用前的值（也 16 字节对齐）

  caller:                    callee:
    SUB SP, SP, #16            STP X29, X30, [SP, #-16]!
    ...                        ...
    BL callee  ──────────→     ; SP 已对齐（caller 保证）
    ...    ←─────────────────  LDP X29, X30, [SP], #16
    ADD SP, SP, #16            RET  ; SP 恢复对齐
```

### SIMD 对齐要求

```asm
; NEON/SIMD 指令可能要求 16/32 字节对齐
ST1 {V0.16B}, [SP]         ; 16 字节存储，SP 必须 16 对齐
STP Q0, Q1, [SP, #-32]!    ; 32 字节存储，SP 必须 32 对齐

; 如果 SP 只有 16 对齐，32 字节对齐的 SIMD 操作会异常
; 解决：额外对齐
SUB SP, SP, #32
AND SP, SP, #~0x1F         ; 32 字节对齐
STP Q0, Q1, [SP]
```

## 与 C 的对照

```c
// C 编译器自动保证栈对齐
void func() {
    int a;           // 编译器在 16 字节对齐的栈上分配
    char buf[10];    // 编译器自动 padding 到 16 倍数
    // SP 始终 16 字节对齐
}

// 内联汇编中破坏对齐 → 危险
void bad_func() {
    asm volatile("sub sp, sp, #8");  // ❌ 破坏对齐
    // 后续的函数调用或局部变量访问可能崩溃
    asm volatile("add sp, sp, #8");
}
```

## 常见错误

1. **SUB SP, SP, #8**：分配非 16 倍数的栈空间 → 后续 STP/LDP 崩溃。
2. **内联汇编中操作 SP**：C 编译器不知道 SP 被改 → 栈帧混乱。
3. **SIMD 对齐不足**：SP 16 对齐但 SIMD 需要 32 对齐 → NEON 指令异常。

## HFT 关联

栈对齐影响性能和正确性：
- 16 字节对齐保证 STP/LDP 单次访问完成 → 不对齐可能拆成两次访存
- 某些 SIMD 指令要求 16/32 字节对齐 → 栈不对齐会导致异常
- HFT 中用 SIMD 加速时需确保栈对齐（编译器自动保证）
- 内联汇编中手动操作 SP 需特别注意 16 字节对齐

```asm
; HFT：SIMD 加速的订单处理函数
; 确保栈对齐 32 字节（NEON Q 寄存器对齐）
process_orders:
    STP X29, X30, [SP, #-32]!   ; 32 字节栈帧（16 的倍数）
    STP D8, D9, [SP, #16]       ; 保存 NEON 寄存器
    ; ... SIMD 处理 ...
    LDP D8, D9, [SP, #16]
    LDP X29, X30, [SP], #32
    RET
```

## 自测题

1. `sub sp, sp, #8` 后执行 `str x0, [sp]` 会发生什么？
<details><summary>答案</summary>
如果 SCTLR 的 SA/SA0 位开启（默认），SP 不是 16 字节对齐 → STR 触发 SP 对齐异常（同步异常，EC=0x26）。即使 SA 关闭，不对齐的访问也可能性能下降或行为未定义。
</details>

2. 函数栈帧为什么通常分配 16 的倍数字节？
<details><summary>答案</summary>
1. AAPCS64 要求 SP 16 字节对齐
2. STP/LDP 操作 16 字节，对齐时单次访存
3. 编译器自动按 16 字节倍数分配栈帧
4. 手动分配非 16 倍数会破坏对齐 → 后续 STP/LDP 异常
</details>

3. 如何在 GDB 中检查 SP 是否 16 字节对齐？
<details><summary>答案</summary>
```gdb
p/x $sp
# 检查低 4 位是否为 0
# 例如 $sp = 0xffff8000a000 → 对齐
#      $sp = 0xffff8000a008 → 不对齐
```
也可以 `p/x $sp & 0xf`，结果为 0 则对齐。
</details>

4. 为什么 SIMD 指令可能需要 32 字节对齐？栈只有 16 对齐怎么办？
<details><summary>答案</summary>
NEON Q 寄存器是 128 位（16字节），STP Q0,Q1 是 32 字节操作，需要 32 字节对齐才能单次总线访问。栈默认只有 16 对齐，解决方案：
1. 手动额外对齐：`AND SP, SP, #~0x1F`（32字节对齐）
2. 分配足够空间保存原始 SP，最后恢复
3. 编译器 `-O2` 通常自动处理 SIMD 对齐
</details>

5. 以下内联汇编有什么问题？如何修复？
```c
void func() {
    asm volatile("sub sp, sp, #8");
    int x = 42;
    asm volatile("add sp, sp, #8");
}
```
<details><summary>答案</summary>
两个问题：
1. `SUB SP, SP, #8` 破坏了 16 字节对齐 → 后续访问可能异常
2. C 编译器不知道 SP 被修改 → 局部变量 `x` 的栈偏移可能错误

修复：不要在内联汇编中直接操作 SP。如果必须分配栈空间，使用 16 的倍数并通知编译器：
```c
void func() {
    int x __attribute__((aligned(16)));
    // 让编译器管理栈
}
```
或使用完整的函数内联汇编，自己管理整个栈帧。
</details>

## 参考与延伸

- 原书 §7.4
- [3.4 STP/LDP](../../chapter-03-a64-load-store/notes/section-0-本章完整概述.md)
- [Ch21 调用约定与栈帧](../../chapter-21-os-topics/notes/section-0-本章完整概述.md)
