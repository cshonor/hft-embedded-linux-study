# 5.8 易错点清单

> 来源：§5.8 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

比较与分支指令在实际编码中最容易犯的错误，以及如何避免。

## 八大易错点

### 1. 有符号 vs 无符号条件后缀选错

**错误**：用 LT/GE 比较地址（无符号数）。

```asm
; BUG：地址比较用了有符号后缀
CMP x0, x1          ; x0=0xFFFF0000, x1=0x00010000
B.LT less           ; LT 看符号位 → x0 被当成负数 → "小于" → 跳转！
                    ; 但实际上 0xFFFF0000 > 0x00010000（无符号）

; 正确：地址比较用 LO/HS
CMP x0, x1
B.LO less           ; LO 看进位标志 → C=0(有借位) → 不跳转 ✓
```

**规则**：
- 地址、大小、长度 → 无符号（LO/HS/HI/LS）
- 有符号整数（可能为负）→ 有符号（LT/GE/GT/LE）
- 不确定时用 EQ/NE（通用）

### 2. 嵌套 BL 忘记保存 X30

**错误**：非叶子函数不保存 LR，导致返回地址丢失。

```asm
; BUG：func 调用了 inner，但没有保存 X30
func:
    BL inner        ; X30 被设为 inner 返回地址
    BL inner2       ; X30 又被覆盖！func 的返回地址已丢失
    RET             ; RET 跳到 inner2 的返回地址，不是 func 的调用者！

; 正确：入口保存 X30
func:
    STP x29, x30, [sp, #-16]!   ; 保存帧指针和返回地址
    BL inner
    BL inner2
    LDP x29, x30, [sp], #16    ; 恢复
    RET                          ; 正确返回到 func 的调用者
```

**判断是否需要保存 X30**：函数内有任何 BL/BLR 指令 → 非叶子函数 → 必须保存。

### 3. CBZ 只能判零，不能比较大小

**错误**：误用 CBZ 做通用比较。

```asm
; BUG：想判断 x0 < 10，但 CBZ 只能判 ==0
CBZ x0, skip        ; 这只判断 x0 == 0，不是 x0 < 10

; 正确：用 CMP + B.LT
CMP x0, #10
B.LT skip           ; x0 < 10 → 跳转
```

**CBZ 的适用场景**：只有 `==0` 和 `!=0` 的判断。其他比较必须用 CMP+B.cond。

### 4. CSEL 条件方向搞反

**错误**：搞混 CSEL 的参数顺序。

```asm
; CSEL Xd, Xn, Xm, cond
;   cond 成立 → Xd = Xn
;   cond 不成立 → Xd = Xm

; BUG：想取 max(x0, x1) 但搞反了
CMP x0, x1
CSEL x2, x1, x0, GE    ; 错！GE 成立时选 x1 → 得到 min 不是 max

; 正确：GE 成立(x0>=x1)时选 x0
CMP x0, x1
CSEL x2, x0, x1, GE    ; 对！GE 成立选 x0 → max(x0, x1)
```

**记忆口诀**："真前假后"——条件为真选第一个源（前），条件为假选第二个源（后）。

### 5. B.cond / CBZ 跳转范围不足

**错误**：目标地址超出跳转范围，链接器报错。

```asm
; B.cond 范围 ±1MB，CBZ 范围 ±1MB，TBZ 范围 ±32KB
B.LT very_far_label     ; 如果 very_far_label 超出 ±1MB → 链接报错

; 解决：用无条件 B 中转
B.LT near_trampoline    ; 条件跳到附近
B continue
near_trampoline:
B very_far_label        ; 无条件 B 范围 ±128MB
continue:
```

### 6. 以为 CMP 会修改操作数

**错误**：以为 CMP x0, x1 会改变 x0 或 x1 的值。

```asm
; CMP 不会修改任何寄存器（结果写入 XZR 丢弃）
MOV x0, #5
MOV x1, #3
CMP x0, x1
; x0 仍然是 5，x1 仍然是 3，只有 NZCV 改变

; 如果需要减法结果，用 SUBS
SUBS x2, x0, x1         ; x2 = x0 - x1 = 2，同时设 NZCV
```

### 7. CMN 方向混淆

**错误**：以为 CMN 做的是减法。

```asm
; CMN x0, x1 做的是 x0 + x1，不是 x0 - x1
; 等价于 CMP x0, (-x1)

; 想判断 x0 == -5：
; 正确：CMN x0, #5（x0 + 5 == 0 等价于 x0 == -5）
CMN x0, #5
B.EQ is_negative_five

; 错误：以为 CMN x0, #5 是 x0 - 5
; 那是 CMP x0, #5
```

### 8. 忘记 CSEL 需要前置 CMP

**错误**：CSEL 读的是 NZCV 标志，如果前一条不是 CMP/SUBS/ADDS，标志可能是旧值。

```asm
; BUG：CSEL 前没有 CMP，用的是过时的标志
MOV x0, #10
CSEL x2, x0, x1, GE    ; GE 条件来自谁？可能是很久以前的 CMP！

; 正确：CSEL 前必须有设标志的指令
CMP x0, x1
CSEL x2, x0, x1, GE    ; GE 来自刚做的 CMP x0, x1
```

## 易错点速查表

| 编号 | 易错点 | 正确做法 | 关键词 |
|------|--------|----------|--------|
| 1 | LT/GE 比较地址 | 地址用 LO/HS | 有符号 vs 无符号 |
| 2 | 嵌套 BL 不保存 X30 | 非叶子函数入口 STP | 保存 LR |
| 3 | CBZ 做大小比较 | 用 CMP+B.cond | CBZ 只判零 |
| 4 | CSEL 参数搞反 | "真前假后" | 条件方向 |
| 5 | 跳转超出范围 | 用 B 中转 | ±1MB/±32KB |
| 6 | 以为 CMP 改寄存器 | CMP 丢弃结果 | 用 SUBS 存结果 |
| 7 | CMN 方向搞混 | CMN 是加法不是减法 | CMN=CMP(-x) |
| 8 | CSEL 忘记前置 CMP | CSEL 前必须有 CMP | 读 NZCV |

## 自测题

1. `csel x0, x1, x2, ge` — x0 最终是什么？
<details><summary>答案</summary>
如果 GE 条件成立（N==V），x0 = x1；否则 x0 = x2。注意 CSEL 的参数顺序：目的、条件真时选的源、条件假时选的源、条件。
</details>

2. 以下代码有什么 bug？
```asm
func:
    bl inner        ; 调用 inner
    bl inner2       ; 再次调用
    ret
```
<details><summary>答案</summary>
第一次 BL inner 会把返回地址存入 X30。第二次 BL inner2 又覆盖 X30。如果 inner 内部调用了其他函数（BL），X30 被进一步覆盖，func 的 RET 会跳到错误地址。必须在入口保存 X30：`stp x29, x30, [sp, #-16]!`。
</details>

3. 为什么 CBZ 不能替代 CMP+B.LT？
<details><summary>答案</summary>
CBZ 只判断是否等于零，不能判断大小关系。`B.LT` 需要先 CMP 设置 NZCV，然后根据 N≠V 跳转。CBZ 无法设置这些比较标志，只能做 ==0 或 !=0 的判断。
</details>

4. 以下代码哪里错了？
```asm
MOV x0, #0xFFFF0000
MOV x1, #0x00010000
CMP x0, x1
B.LT less           ; 期望 x0 > x1 时不跳转
```
<details><summary>答案</summary>
x0=0xFFFF0000 和 x1=0x00010000 作为地址（无符号）比较时 x0 > x1，不应该跳转。但 B.LT 做有符号比较，0xFFFF0000 的符号位为1被当成负数，所以 LT 成立 → 错误跳转。应改为 `B.LO less`（无符号小于）。
</details>

5. 以下代码能正确实现 `x2 = abs(x0)` 吗？如果不能，错在哪？
```asm
CNEG x2, x0, MI
```
<details><summary>答案</summary>
不能正确工作。CNEG 读取 NZCV 标志中的 MI 条件（N=1），但前面没有 CMP 或其他设标志的指令。N 标志可能是上次运算留下的旧值，CNEG 会根据错误的标志做判断。正确写法：
```asm
CMP x0, #0
CNEG x2, x0, MI    ; 先 CMP 设标志，再 CNEG 读取
```
</details>

## 参考与延伸

- 原书 §5.8
- [5.1 比较指令](01-compare.md)
- [5.2 CSEL](02-csel.md)
- [5.3 跳转指令](03-branch.md)
- [5.4 条件后缀](04-condition-suffix.md)
- [3.4 STP/LDP](../../chapter-03-a64-load-store/notes/section-0-本章完整概述.md)
