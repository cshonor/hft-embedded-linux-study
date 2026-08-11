# 5.3 跳转指令全览

> 来源：§5.3 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

AArch64 所有跳转指令的分类、使用场景、以及它们对 LR(X30) 的影响。重点掌握条件跳转 B.cond 与 NZCV 的关系、循环的两种写法、以及 b.lt vs b.lo 这个最高频踩坑点。

## 核心要点

### B 是什么

B = Branch，分支/跳转。改变 PC（程序计数器），不去取下一条顺序指令，跳到别的地址执行。

分两大类：
- **无条件跳转**：`B label`，一定跳，不看标志
- **条件跳转**：`B.cond label`，看 NZCV 标志，满足条件才跳，不满足就顺序往下跑

> **写法注意**：AArch64 是点分隔 `B.eq`，不是 `BEQ`（那是 ARM32 汇编写法）。

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

### B / BL / RET 三者辨析（记忆口诀）

```
B.cond  → 条件分支，看标志跳
B       → 无条件跳，不保存返回地址，用于循环、if 分支
BL      → 跳转 + 保存返回地址到 X30(LR)，用于函数调用
RET     → 拿 X30(LR) 的值放回 PC，用于函数返回
```

> **一句话记忆**：`B.cond` 条件分支；`B` 无条件；`BL` 调用函数；`RET` 函数返回。

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

#### B.cond 四条速查（最高频）

| 指令 | 含义 | 依据标志 | 典型场景 |
|------|------|----------|----------|
| B.EQ | 等于就跳 | Z=1 | 相等比较 |
| B.NE | 不等于就跳 | Z=0 | 不相等比较 |
| B.LT | 有符号数小于就跳 | N ≠ V | 带正负号比较 |
| B.LO | 无符号数小于就跳 | C=0 | 纯正数、地址、size |

> **来源**：前面 `CMP` / `SUBS` 算完，修改 NZCV，后面条件跳转读取 NZCV 做判断。

#### 完整条件后缀高频清单

| 后缀 | 含义 | 条件 | 类型 |
|------|------|------|------|
| B.EQ | 等于 | Z=1 | 通用 |
| B.NE | 不等于 | Z=0 | 通用 |
| B.LT | 有符号小于 | N≠V | 有符号 |
| B.LE | 有符号小于等于 | N≠V or Z=1 | 有符号 |
| B.GT | 有符号大于 | N==V and Z=0 | 有符号 |
| B.GE | 有符号大于等于 | N==V | 有符号 |
| B.LO | 无符号小于 | C=0 | 无符号 |
| B.LS | 无符号小于等于 | C=0 or Z=1 | 无符号 |
| B.HI | 无符号大于 | C=1 and Z=0 | 无符号 |
| B.HS | 无符号大于等于 | C=1 | 无符号 |
| B.MI | 负数 | N=1 | 标志检测 |
| B.PL | 非负数 | N=0 | 标志检测 |

> 完整推导和 16 种条件后缀详见 [5.4 条件后缀速查](04-condition-suffix.md)。

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

### 循环的两种典型写法

#### 写法 1：先判断再循环（while）

```asm
loop_label:
    CMP x0, #0
    B.EQ exit            ; 如果 x0 等于 0，直接跳出循环
    ; ... 循环体工作 ...
    SUB x0, x0, #1
    B loop_label         ; 无条件跳转，回到开头
exit:
```

#### 写法 2：先干活后判断（do-while，嵌入式汇编最常见）

```asm
loop_label:
    ; ... 循环体，先做事情 ...
    SUBS x0, x0, #1      ; S 后缀，自动更新 NZCV，省去 CMP
    B.NE loop_label      ; x0 ≠ 0 就跳回开头继续循环
```

> **HFT 要点**：`SUBS` 直接修改标志，不需要额外写 `CMP`，少一条指令，降低延迟。这是 ARM 循环的标准优化。

### 极易踩坑：B.LT vs B.LO

**B.LT**：给有符号补码用，比如 `-5 < 3`
**B.LO**：给无符号用，内存地址、缓冲区长度、size，永远用 B.LO / B.HS

```
坑例子：
寄存器比特：0xFFFFFFFFFFFFFFFF（就是 -1 的补码，也是 -5 的补码是 0xFFFFFFFFFFFFFFFB）

当作有符号：-1，B.LT #3 → 成立，小于 3
当作无符号：18446744073709551615（极大正数），B.LO #3 → 不成立，不小于 3

一模一样的二进制，选错跳转指令逻辑直接 bug。
```

| 场景 | 正确后缀 | 错误后缀 | 原因 |
|------|----------|----------|------|
| 地址比较 `ptr < end` | B.LO | B.LT ✗ | 地址无符号，高位为 1 会被当负数 |
| size 比较 `len < 10` | B.LO | B.LT ✗ | 长度无符号 |
| 价差比较 `diff < 0` | B.LT | B.LO ✗ | 价差可正可负，有符号 |
| 循环计数 `i < count` | B.LT | B.LO ✓ 也可 | 计数器非负，两者结果相同，但习惯用 LT |

### 底层原理：CMP → NZCV → B.cond

```
CMP x1, x2
  → 执行 SUBS XZR, x1, x2（运算结果丢弃到零寄存器，只改写 NZCV）

CPU 硬件读取 NZCV 四个 bit：
  B.EQ → 看 Z=1？
  B.LT → 看 N≠V？
  B.LO → 看 C=0？

满足条件 → 修改 PC，跳到目标地址
不满足   → PC 正常 +4，继续执行下一条指令
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
4. **B.LT 比地址**：地址是无符号数，用 B.LT 会导致高位地址被当负数，比较结果错误。

## HFT 关联

跳转指令的延迟差异影响热路径：
- B/BL 是 PC 相对跳转，可被分支预测器缓存 → 预测正确 ~0-1 cycle
- BR/BLR 是间接跳转（寄存器目标），预测更难 → 可能 ~5+ cycles
- 函数指针/虚函数调用用 BLR → 延迟不稳定
- 尾调用优化用 B 替代 BL+RET → 省一层栈帧和返回开销
- HFT 热路径应尽量减少间接调用（BLR），改用直接调用（BL）
- 循环用 SUBS+B.NE 省掉 CMP → 每次迭代少 1 cycle

```asm
; HFT 反模式：函数指针导致间接跳转
LDR x0, [=process_order_v1]   ; 运行期选择函数
BLR x0                         ; 间接调用 → 难预测 → ~5+ cycles

; HFT 正模式：编译期确定函数地址
BL process_order               ; 直接调用 → 可预测 → ~1 cycle

; HFT 循环优化：SUBS 省掉 CMP
; ❌ 多一条 CMP
loop_bad:
    ; ... work ...
    SUB x0, x0, #1
    CMP x0, #0
    B.NE loop_bad

; ✅ SUBS 一步到位
loop_good:
    ; ... work ...
    SUBS x0, x0, #1           ; 减 1 + 设标志
    B.NE loop_good            ; 直接跳转
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

6. **思考题**：以下代码会跳到 ok1 还是 ok2？
```asm
MOV x0, #-2        ; x0 = 0xFFFFFFFFFFFFFFFE
MOV x1, #2         ; x1 = 0x0000000000000002
CMP x0, x1
B.LT ok1
B.LO ok2
```
<details><summary>答案</summary>
跳到 **ok1**。

分析：CMP x0, x1 执行 x0 - x1 = (-2) - 2 = -4。
- N=1（结果符号位为 1，负数）
- Z=0（结果非零）
- C=0（有借位：无符号视角 0xFFFFFFFFFFFFFFFE < 0x0000000000000002）
- V=0（无溢出：-2-2=-4 在 int64 范围内）

B.LT 条件：N ≠ V → 1 ≠ 0 → **成立** → 跳到 ok1。

B.LO 根本不会被执行到，因为 B.LT 已经跳转了。

但从逻辑验证：如果 B.LT 不跳（假设），B.LO 条件 C=0 → 也成立 → 会跳 ok2。这说明同一个 CMP 结果，有符号和无符号判断可以同时为真——因为 -2 在有符号下确实 < 2，而 0xFFFFFFFFFFFFFFFE 在无符号下也确实 < 2（C=0 表示借位）。

**关键教训**：选对条件后缀取决于你的业务语义（有符号 vs 无符号），而不是"哪个会跳"。
</details>

## 参考与延伸

- 原书 §5.3
- [3.4 STP/LDP 栈帧](../../chapter-03-a64-load-store/notes/section-0-本章完整概述.md)
- [Ch11 ERET 异常返回](../../chapter-11-exception-handling/notes/section-0-本章完整概述.md)
- [5.4 条件后缀速查（完整 16 种）](04-condition-suffix.md)
- [5.6 典型代码模式（循环/无分支等）](06-patterns.md)
- [5.8 易错点清单](08-pitfalls.md)
