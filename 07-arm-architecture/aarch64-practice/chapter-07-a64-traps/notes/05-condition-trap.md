# 7.5 条件执行陷阱

> 来源：§7.5 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

从 AArch32（ARMv7）迁移到 AArch64（ARMv8）时，条件执行机制的差异导致的陷阱。AArch64 取消了 IT 块和大部分条件指令后缀。

## 核心要点

### AArch32 vs AArch64 条件执行

| 特性 | AArch32 (ARMv7) | AArch64 (ARMv8) |
|------|-----------------|------------------|
| 条件后缀 | 几乎所有指令可加后缀 | 仅 B.cond/CSEL/CSINC/CCMP |
| IT 块 | IT 指令让下 4 条指令条件执行 | **无 IT 块** |
| 示例 | `MOVEQ R0, #1`（条件 MOV） | 不支持，用 CSEL 替代 |
| 预测执行 | 条件指令都预测执行 | 用 CSEL 替代（无分支） |

### AArch32 的条件执行（ARMv7）

```asm
; ARMv7：几乎所有指令都可以加条件后缀
CMP R0, R1
MOVEQ R0, #1        ; EQ 成立 → R0 = 1
MOVNE R0, #0        ; NE 成立 → R0 = 0
ADDGT R2, R2, #1    ; GT 成立 → R2++
LDRLS R3, [R4]      ; LS 成立 → R3 = *R4

; IT 块（If-Then）
CMP R0, #0
ITE EQ              ; If-Then-Else: EQ
MOVEQ R0, #1        ; Then (EQ): R0 = 1
MOVNE R0, #0        ; Else (NE): R0 = 0
```

### AArch64 的替代方案

```asm
; AArch64：用 CSEL 替代条件 MOV
CMP X0, X1
CSEL X0, X1, X2, EQ    ; EQ → X0=X1, NE → X0=X2

; AArch64：用 CSET 替代条件置 1
CMP X0, X1
CSET X0, EQ            ; EQ → X0=1, NE → X0=0

; AArch64：用 CINC 替代条件自增
CMP X0, X1
CINC X2, X2, GT        ; GT → X2=X2+1, LE → X2=X2

; AArch64：用 CCMP 替代条件比较
; CCMP：条件比较——如果前一条件成立才执行 CMP
CMP X0, #0
CCMP X1, #0, #0, NE    ; NE(X0!=0) → CMP X1, #0
                         ; EQ(X0==0) → 设标志为 #0(NZCV=0000)
; 用于级联条件：if (x != 0 && y != 0)
```

### 迁移示例

```asm
; AArch32 代码：
CMP R0, #10
ITTE GT
ADDGT R0, R0, #1     ; GT → R0++
ADDGT R1, R1, #1     ; GT → R1++
MOVLE R2, #0         ; LE → R2=0

; AArch64 等价：
CMP X0, #10
B.LE skip
ADD X0, X0, #1       ; GT → X0++
ADD X1, X1, #1       ; GT → X1++
B continue
skip:
MOV X2, #0           ; LE → X2=0
continue:

; 或者用 CSEL（无分支版本，仅适用于简单赋值）
CMP X0, #10
CSEL X2, X3, XZR, GT  ; GT → X2=X3, LE → X2=0（需要预设 X3）
```

### CCMP 详解

```asm
; CCMP（Conditional Compare）—— AArch64 特有
; 语法：CCMP Xn, Xm, #nzcv, cond
; 如果 cond 成立 → 执行 CMP Xn, Xm（设 NZCV）
; 如果 cond 不成立 → NZCV = #nzcv（直接设置标志）

; 用途：实现 && 的短路求值
; C: if (a > 0 && b > 0)
CMP X0, #0            ; 比较 a > 0
CCMP X1, #0, #0b0000, GT  ; GT(a>0) → CMP X1, #0
                            ; LE(a<=0) → NZCV=0000(Z=1, 相当于 false)
B.LE both_positive      ; 两个条件都成立 → GT

; 等价于：
CMP X0, #0
B.LE skip              ; a <= 0 → 跳过
CMP X1, #0              ; b > 0?
B.LE skip
; both positive
skip:
```

### 为什么取消 IT 块

```
AArch32 IT 块的问题：
1. 解码复杂：需要跟踪 IT 状态寄存器（4 位条件链）
2. 乱序不友好：IT 块内的指令必须按序执行
3. 断点困难：GDB 单步进入 IT 块时行为复杂
4. 安全问题：IT 块可被用于侧信道攻击（条件预测）

AArch64 的改进：
1. 简化指令集：只有少数专用条件指令
2. 乱序友好：CSEL 是独立指令，不依赖前序状态
3. 调试友好：每条指令独立可单步
4. 安全性：减少条件预测的攻击面
```

## 与 C 的对照

```c
// C 的三元运算符 → AArch64 CSEL
int val = (a > b) ? a : b;
// → CMP + CSEL

// C 的逻辑与 → AArch64 CCMP
if (a > 0 && b > 0) { ... }
// → CMP + CCMP + B.cond

// C 的 if-else → AArch64 分支
if (a > b) { ... } else { ... }
// → CMP + B.GT + B
```

## 常见错误

1. **从 ARMv7 直接翻译条件指令**：`MOVEQ` → AArch64 无等价单条指令，需改写为 CSEL 或分支。
2. **以为 AArch64 支持条件 ADD/SUB**：`ADDGT X0, X0, #1` 在 AArch64 非法，用 `CINC X0, X0, GT`。
3. **混淆 CSEL 和分支的适用场景**：CSEL 只能选值，不能执行不同代码块。复杂逻辑仍需分支。

## HFT 关联

从 32 位迁移到 64 位时的常见问题：
- ARMv7 的条件执行可以消除分支 → AArch64 需用 CSEL 替代
- 迁移老代码时不能直接翻译 `moveq` → 需改写为 CSEL 或分支
- AArch64 的 CSEL 虽然不如条件执行通用，但覆盖了大部分场景
- 性能上 CSEL 仍然无分支，延迟可预测

## 自测题

1. AArch64 为什么取消了 IT 块？
<details><summary>答案</summary>
IT 块让 1-4 条指令条件执行，增加解码复杂度（需跟踪 IT 状态），且对乱序执行不友好。AArch64 简化指令集，用 CSEL/CSET/CCMP 等专用条件指令替代，更适合超标量乱序流水线。
</details>

2. AArch32 的 `moveq r0, #1` 在 AArch64 怎么写？
<details><summary>答案</summary>
```asm
; 方法1：CSEL（无分支）
mov x1, #1
mov x2, #0    ; 或保留原值
csel x0, x1, x2, eq

; 方法2：CSET（如果只需要 0/1）
cset x0, eq   ; eq → x0=1, ne → x0=0
```
</details>

3. CSEL 和条件分支哪个更适合 HFT？
<details><summary>答案</summary>
CSEL 更适合。CSEL 始终 1 cycle，无分支预测失败风险。条件分支预测正确时 ~0-1 cycle，但预测失败时 ~20 cycle（流水线冲刷）。HFT 追求延迟可预测性，CSEL 更优。但 CSEL 只能选择值，不能跳转到不同代码块。
</details>

4. 用 CCMP 实现 `if (x > 0 && y < 10)` 的无分支条件判断。
<details><summary>答案</summary>
```asm
CMP X0, #0                  ; x > 0?
CCMP X1, #10, #0b1100, GT   ; GT(x>0) → CMP X1, #10
                              ; LE(x<=0) → NZCV=1100(N=1,V=1 → LT成立 → 不跳)
B.LT both_true               ; x>0 AND y<10 → 跳转
```
CCMP X1, #10 在 x>0 时执行（比较 y 和 10），y<10 → LT 成立。如果 x<=0，直接设 NZCV=1100（N=1,V=1 → N==V → GE，不是 LT）→ B.LT 不跳转。实现了 && 的短路求值。
</details>

5. AArch32 的 `ADDEQ R0, R0, #1` 在 AArch64 的等价写法？
<details><summary>答案</summary>
```asm
; 方法1：CINC（最简洁）
CINC X0, X0, EQ    ; EQ → X0=X0+1, NE → X0=X0

; 方法2：CSEL + ADD
MOV X1, #1
CSEL X2, X1, XZR, EQ  ; EQ → X2=1, NE → X2=0
ADD X0, X0, X2        ; X0 += X2

; CINC 是方法2的编码别名，更简洁
```
</details>

## 参考与延伸

- 原书 §7.5
- [5.2 CSEL/CSET/CCMP](../../chapter-05-a64-compare-branch/notes/02-csel.md)
- [5.4 条件后缀](../../chapter-05-a64-compare-branch/notes/04-condition-suffix.md)
