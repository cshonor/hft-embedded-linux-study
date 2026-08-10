# 5.3 跳转指令全览

> 来源：§5.3 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

AArch64 所有跳转指令的分类、使用场景、以及它们对 LR(X30) 的影响。

## 核心要点

### 跳转指令分类表

| 指令 | 全称 | 作用 | 保存 LR | 寻址方式 | 跳转范围 |
|------|------|------|---------|----------|----------|
| B label | Branch | 无条件跳转 | 否 | PC 相对（立即数） | ±128MB |
| B.cond label | Branch Conditional | 条件跳转 | 否 | PC 相对（立即数） | ±1MB |
| BL label | Branch with Link | 调用函数 | 是(X30) | PC 相对（立即数） | ±128MB |
| BR Xn | Branch Register | 寄存器跳转 | 否 | 寄存器间接 | 任意 |
| BLR Xn | Branch with Link Register | 寄存器调用 | 是(X30) | 寄存器间接 | 任意 |
| RET {Xn} | Return | 函数返回 | 否(读X30) | 寄存器间接 | 任意 |
| ERET | Exception Return | 异常返回 | 否(读ELR) | 特殊 | ELR_ELx |

### 无条件跳转 B

```asm
; 基本跳转
B label               ; 跳转到 label

; 常见于循环
loop:
    ; ... 循环体 ...
    B loop            ; 无条件跳回循环头

; 尾调用：用 B 替代 BL+RET
; 被调函数的最后一条指令是 B another_func
; 而不是 BL another_func; RET
; 省一层栈帧，返回时直接回到原调用者
```

### 条件跳转 B.cond

```asm
; 先用 CMP 设置标志，再条件跳转
CMP x0, x1
B.EQ equal            ; x0 == x1 → 跳转
B.LT less             ; x0 < x1 (有符号) → 跳转
B.GE greater_equal    ; x0 >= x1 (有符号) → 跳转

; 也可用 SUBS 直接设标志
SUBS x0, x0, #1       ; x0--，同时设标志
B.GT loop             ; x0 > 0 → 继续循环
```

### 函数调用 BL / BLR

```asm
; BL：调用标签地址的函数
BL printf             ; 调用 printf，返回地址存入 X30

; BLR：调用寄存器中地址的函数（函数指针/虚函数）
LDR x0, [callback_ptr]
BLR x0                ; 通过函数指针调用

; BL 的等价操作：
; X30 = PC + 4        （保存返回地址）
; PC = label           （跳转到目标）
```

### 函数返回 RET

```asm
; 默认用 X30(LR) 返回
RET                    ; PC = X30

; 指定其他寄存器（用于尾调用优化/协程）
RET x1                 ; PC = x1

; 叶子函数（不调用其他函数）可以省略栈帧
leaf_func:
    MOV x0, #42
    RET                ; 直接用 X30 返回，无需保存

; 非叶子函数必须保存 X30
non_leaf_func:
    STP x29, x30, [sp, #-16]!  ; 保存帧指针和返回地址
    ; ... 调用其他函数 ...
    LDP x29, x30, [sp], #16    ; 恢复
    RET
```

### 异常返回 ERET

```asm
; ERET 从异常处理返回，恢复 PC 和 PSTATE
; PC ← ELR_ELx（异常链接寄存器，保存被异常打断的地址）
; PSTATE ← SPSR_ELx（保存的处理器状态）
; 同时从当前异常等级返回到低一级

ERET                   ; 只在 EL1 以上使用
```

### 跳转范围详解

| 指令 | 偏移编码 | 范围 | 说明 |
|------|----------|------|------|
| B | 26 位 × 4 | ±128MB | 大多数函数内跳转足够 |
| B.cond | 19 位 × 4 | ±1MB | 条件跳转范围较小 |
| BL | 26 位 × 4 | ±128MB | 与 B 相同 |
| CBZ/CBNZ | 19 位 × 4 | ±1MB | 与 B.cond 相同 |
| TBZ/TBNZ | 14 位 × 4 | ±32KB | 范围最小 |

```asm
; 如果 B.cond 的目标超出 ±1MB 范围，需要中转
B.LT far_label         ; 如果 far_label 超出范围 → 链接器报错

; 解决方法：用无条件 B 中转
B.LT near_trampoline
B continue
near_trampoline:
B far_label            ; 无条件 B 范围 ±128MB
continue:
```

## BL 与栈帧的关系

```
调用 func() 的完整栈帧流程：

main:                      func:
  ...                        STP x29, x30, [sp, #-16]!  ← 保存 LR
  BL func  ──────────────→  ...                          ← 函数体
  ...    ←────────────────  ...  BL helper ──→ helper:
  RET                        LDP x29, x30, [sp], #16     ← 恢复 LR
                             RET ──────────────────────→  ...
                                                              RET

关键：BL 把返回地址存入 X30。如果 func 内部又 BL helper，
X30 会被覆盖。所以 func 必须在入口保存 X30。
```

## 与 C 的对照

| C 代码 | AArch64 汇编 |
|--------|-------------|
| `goto label;` | `B label` |
| `if (cond) goto label;` | `B.cond label` |
| `func();` | `BL func` |
| `func_ptr();` | `BLR x0` |
| `return;` | `RET` |
| 尾调用 `return func();` | `B func`（省去 RET） |

## 常见错误

1. **嵌套 BL 忘记保存 X30**：非叶子函数入口必须 `STP x29, x30, [sp, #-N]!`。
2. **B.cond 超范围**：±1MB 可能不够，需用 B 中转。
3. **混淆 BR 和 B**：B 是 PC 相对（编译期确定地址），BR 是寄存器间接（运行期确定地址）。

## HFT 关联

跳转指令的延迟差异影响热路径：
- B/BL 是 PC 相对跳转，可被分支预测器缓存 → 预测正确 ~0-1 cycle
- BR/BLR 是间接跳转（寄存器目标），预测更难 → 可能 ~5+ cycles
- 函数指针/虚函数调用用 BLR → 延迟不稳定
- 尾调用优化用 B 替代 BL+RET → 省一层栈帧和返回开销
- HFT 热路径应尽量减少间接调用（BLR），改用直接调用（BL）

```asm
; HFT 反模式：函数指针导致间接跳转
LDR x0, [=process_order_v1]   ; 运行期选择函数
BLR x0                         ; 间接调用 → 难预测 → ~5+ cycles

; HFT 正模式：编译期确定函数地址
BL process_order               ; 直接调用 → 可预测 → ~1 cycle
```

## 自测题

1. BL 和 B 的区别？
<details><summary>答案</summary>
BL（Branch with Link）在跳转前把下一条指令地址存入 X30(LR)，用于函数返回。B（Branch）不保存返回地址，用于无条件跳转/循环。
</details>

2. 为什么嵌套函数调用必须保存 X30？
<details><summary>答案</summary>
BL 会覆盖 X30 为新的返回地址。如果外层函数不保存 X30，内层 BL 返回后 X30 已变，外层 RET 会跳到错误地址。所以非叶子函数入口必须 STP x29, x30 压栈保存。
</details>

3. `ret x1` 和 `ret`（默认）的区别？
<details><summary>答案</summary>
`ret` 默认用 X30(LR) 作为返回地址。`ret x1` 用 x1 作为返回地址。非默认形式用于替代调用约定，如尾调用优化或协程切换。
</details>

4. 为什么函数指针调用（BLR）比直接调用（BL）慢？
<details><summary>答案</summary>
BL 的目标地址在编译期确定，分支预测器可以缓存该地址，预测准确率高。BLR 的目标在寄存器中，运行期才知道，预测器无法提前缓存，预测失败时需冲刷流水线（~5+ cycles）。HFT 热路径应尽量用直接调用。
</details>

5. B.cond 的跳转范围只有 ±1MB，如果目标更远怎么办？
<details><summary>答案</summary>
用无条件 B 做中转：
```asm
B.LT near_trampoline     ; 条件跳转到附近的中转点
B skip                   ; 条件不满足时跳过
near_trampoline:
B far_label              ; 无条件 B 范围 ±128MB
skip:
```
编译器/汇编器通常会自动处理这种中转。
</details>

## 参考与延伸

- 原书 §5.3
- [3.4 STP/LDP 栈帧](../../chapter-03-a64-load-store/notes/section-0-本章完整概述.md)
- [Ch11 ERET 异常返回](../../chapter-11-exception-handling/notes/section-0-本章完整概述.md)
- [5.6 典型代码模式](06-patterns.md)
