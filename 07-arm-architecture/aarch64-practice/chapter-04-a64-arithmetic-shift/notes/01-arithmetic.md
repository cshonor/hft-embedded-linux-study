# 4.1 算术指令

> 来源：§4.1 · 精读 · [章总览](section-0-本章完整概述.md) · [补码专篇](../../SIGNED-UNSIGNED.md)

## 本节讲什么

ADD/SUB/CMP/MUL/SDIV/UDIV 等全部整数算术指令，以及补码与运算的关系。

---

## 补码：ARM64 数据表示的基石

ARM64 寄存器和内存里**只有二进制比特流**，不标记 signed/unsigned。有符号数用**补码（two's complement）**存放：

| 概念 | 说明 |
|------|------|
| **正数补码** | 原码本身，如 `+5` → `0x0000000000000005` |
| **负数补码** | 取反加一，如 `-5` → `~5 + 1 = 0xFFFFFFFFFFFFFFFB` |
| **零** | 全零 `0x0000000000000000` |
| **-1** | 全一 `0xFFFFFFFFFFFFFFFF` |
| **最小值** | `INT64_MIN = 0x8000000000000000`（`-2^63`） |
| **最大值** | `INT64_MAX = 0x7FFFFFFFFFFFFFFF`（`2^63 - 1`） |

> 补码的核心优势：**加减法统一**。有符号和无符号的 ADD/SUB 用同一条指令、同样的二进制运算，只有溢出检测方式不同。运算出来之后，再靠 NZCV 标志区分：**V 标志**判断有符号溢出，**C 标志**判断无符号进位。

---

## 加减法指令

| 指令 | 作用 | 改 NZCV？ |
|------|------|-----------|
| `ADD` | `Rd = Rn + Op2` | ❌ |
| `ADDS` | 同上 | ✅ 设置 NZCV |
| `ADC` | `Rd = Rn + Op2 + C`（带进位加） | ❌ |
| `ADCS` | 同上 | ✅ |
| `SUB` | `Rd = Rn - Op2` | ❌ |
| `SUBS` | 同上 | ✅ |
| `SBC` | `Rd = Rn - Op2 - NOT(C)`（带借位减） | ❌ |
| `SBCS` | 同上 | ✅ |
| `NEG` | `Rd = -Op2`（伪指令 ≡ `SUB XZR, XZR, Op2`） | ❌ |
| `NEGS` | 同上 | ✅ |
| `NGC` | `Rd = -Op2 - NOT(C)` | ❌ |
| `CMP` | 比较 ≡ `SUBS XZR, Rn, Op2`（结果丢弃） | ✅ |
| `CMN` | 负比较 ≡ `ADDS XZR, Rn, Op2` | ✅ |

### S 后缀铁则（反复记）

> **带 S 改标志；不带 S 只算数值。**
> S = **Set flags**，不是 signed。（`LDRSB` 的 S 才是 Sign → [S-SUFFIX.md](../../S-SUFFIX.md)）

```
┌─────────────────────────────────────────────────────────────────┐
│  ⚠️ 两处 S 含义完全不同，不要搞混：                               │
│                                                                 │
│  算术指令 S 后缀（ADDS / SUBS）  → Set flags，设置 NZCV 标志位    │
│  加载指令 S 后缀（LDRSB / LDRSH） → Sign，符号扩展               │
│                                                                 │
│  ADDS x0, x1, x2    ; S = 设置标志                               │
│  LDRSB x0, [x1]     ; S = 符号扩展（字节→64位）                  │
└─────────────────────────────────────────────────────────────────┘
```

### ADD 三种操作数形式

```asm
; 1. 寄存器 + 寄存器
add  x0, x1, x2           ; x0 = x1 + x2

; 2. 寄存器 + 立即数（12位无符号，0~4095，或 <<12 = 0~0xFFF000）
add  x0, x1, #100          ; x0 = x1 + 100

; 3. 寄存器 + (寄存器 << 移位)  ← 内嵌移位！
add  x0, x1, x2, lsl #3   ; x0 = x1 + (x2 << 3) = x1 + x2 * 8
```

> 立即数范围：`ADD` 立即数是 **12 位无符号**（0~4095），可选 `LSL #12` 左移（变为 0x1000 的倍数）。超出范围需用 `MOVZ`/`MOVK` 组合。

### CMP 详解

```asm
cmp  x0, x1               ; 等价于 subs xzr, x0, x1
                          ; x0 - x1 的结果丢弃，只更新 NZCV
```

CMP **不写回结果**（写 `XZR`），只设置标志。之后跟条件分支：

### CMN（负数比较）

```asm
cmn  x0, x1               ; 等价于 adds xzr, x0, x1
                          ; 计算 x0 + x1，看是否为 0
```

用于"判断 x0 是否等于 -x1"的场景。

### CMP 后条件分支速查表

| 条件码 | 助记符 | 含义 | 判断的标志 |
|--------|--------|------|-----------|
| EQ | `b.eq` | 相等 | Z = 1 |
| NE | `b.ne` | 不等 | Z = 0 |
| LT | `b.lt` | 有符号小于 | N ≠ V |
| LO/CC | `b.lo` | 无符号小于 | C = 0 |
| GE | `b.ge` | 有符号 ≥ | N = V |
| HS/CS | `b.hs` | 无符号 ≥ | C = 1 |
| LE | `b.le` | 有符号 ≤ | !(N = V) ∨ Z |
| HI | `b.hi` | 无符号 > | C = 1 ∧ Z = 0 |
| GT | `b.gt` | 有符号 > | N = V ∧ Z = 0 |

> **易错点**：`b.lt`（有符号）和 `b.lo`（无符号）判断的标志完全不同。比较地址/指针大小时用 `b.lo`/`b.hs`，比较带符号数值时用 `b.lt`/`b.ge`。

### ADC/SBC 多精度运算

ADC：带进位加法，加 C 标志。SBC：带借位减法。

128 位加法（两个 64 位寄存器拼成一个 128 位数）：

```asm
; [x1:x0] + [x3:x2] → [x1:x0]   （x0=低64位, x1=高64位）
adds x0, x0, x2            ; 低 64 位加，C 标志记录进位
adc  x1, x1, x3            ; 高 64 位加 + 低位的进位
```

128 位减法：

```asm
; [x1:x0] - [x3:x2] → [x1:x0]
subs x0, x0, x2            ; 低 64 位减，C 标志记录借位
sbc  x1, x1, x3            ; 高 64 位减 - 借位
```

> **HFT 场景**：ADC/SBC 在加密算法（如 SHA-256 大数运算）中使用，HFT 交易逻辑中较少直接使用。

---

## 乘法指令

| 指令 | 作用 | 结果位宽 |
|------|------|---------|
| `MUL` | `Rd = Rn * Rm`（低 64 位） | 64 bit |
| `MADD` | `Rd = Ra + Rn * Rm`（乘加） | 64 bit |
| `MSUB` | `Rd = Ra - Rn * Rm`（乘减） | 64 bit |
| `SMULH` | 有符号乘法高 64 位 | 64 bit |
| `UMULH` | 无符号乘法高 64 位 | 64 bit |

### 补码与乘法的关系

| 乘法 | 低 64 位 | 高 64 位 |
|------|---------|---------|
| 有符号（`SMULH`） | ✅ 与无符号**相同** | ❌ 不同，需 `SMULH` |
| 无符号（`UMULH`） | ✅ 与有符号**相同** | ❌ 不同，需 `UMULH` |

> **关键**：`MUL` 取低 64 位时，有符号和无符号乘法结果相同（补码性质）。但如果要 128 位完整结果，高位必须分 `SMULH`/`UMULH`。

### MADD 乘加指令

```asm
; 计算 x0 = x3 + x1 * x2  ← 一条指令完成乘加
madd x0, x1, x2, x3       ; x0 = x3 + x1 * x2

; 如果不需要加数，用 XZR 代替
mul  x0, x1, x2           ; 等价于 madd x0, x1, x2, xzr
```

> `MADD` 比 `MUL + ADD` 两条指令更高效，编译器优先使用 `MADD`。

### MSUB 乘减指令

```asm
; 计算 x0 = x3 - x1 * x2
msub x0, x1, x2, x3       ; x0 = x3 - x1 * x2
```

> HFT 场景：`MADD`/`MSUB` 用于价格计算（`总价 = 基础费 + 单价 × 数量`），一条指令搞定。减少指令条数、降低延迟。

---

## 除法指令

| 指令 | 作用 | 有符号？ | 除零行为 |
|------|------|---------|---------|
| `SDIV` | `Rd = Rn / Rm` | ✅ 有符号 | 结果为 0 |
| `UDIV` | `Rd = Rn / Rm` | ❌ 无符号 | 结果为 0 |

> ⚠️ **除法是加减乘除中唯一必须区分有符号/无符号的指令。**

### SDIV 向零截断

```asm
; 有符号除法
mov  x0, #-7
mov  x1, #2
sdiv x2, x0, x1           ; x2 = -7 / 2 = -3（向零截断，不是 -4！）

; 无符号除法
mov  x0, #0xFFFFFFFFFFFFFFF9  ; = -7 的补码，但当作无符号 = 18446744073709551609
mov  x1, #2
udiv x2, x0, x1           ; x2 = 18446744073709551609 / 2 = 9223372036854775804
```

| 被除数 | 除数 | SDIV 结果 | 说明 |
|--------|------|----------|------|
| 7 | 2 | 3 | 3.5 → 截断 → 3 |
| -7 | 2 | -3 | -3.5 → 向零截断 → -3（不是 -4） |
| 7 | -2 | -3 | 3.5 → 截断 → -3 |
| -7 | -2 | 3 | -3.5 → 向零截断 → 3 |

> **向零截断**（truncation toward zero）= 丢掉小数部分，不四舍五入。

### 除零不抛异常 ⚠️

ARM64 除法**不产生异常**，除零时结果为 0（不 trap）：

```asm
mov  x0, #10
mov  x1, #0
udiv x2, x0, x1           ; x2 = 0（不 crash，不 trap）
```

> ⚠️ 这跟 x86 不同（x86 除零触发 #DE 异常）。软件必须自行检查除数是否为 0。

```c
// HFT 业务代码必须手动校验除数
inline int64_t safe_div(int64_t a, int64_t b) {
    if (b == 0) return 0;          // ARM64 除零返回 0，但业务上要明确处理
    return a / b;                   // 编译器生成 sdiv
}
```

---

## 取负指令

```asm
neg  x0, x1               ; x0 = -x1 ≡ sub x0, xzr, x1
negs x0, x1               ; 同上 + 设置 NZCV
ngc  x0, x1               ; x0 = -x1 - NOT(C)（带借位取负）
```

> `NEG` 是伪指令，汇编器展开为 `SUB XZR, ...`。

---

## 补码运算总结表（必背）

| 运算 | 有无符号是否共用指令 | 备注 |
|------|---------------------|------|
| **加法 ADD** | ✅ 共用 | 看 V 判断有符号溢出，C 判断无符号进位 |
| **减法 SUB** | ✅ 共用 | 同上 |
| **乘法（低64位）MUL** | ✅ 共用 | 补码数学性质决定 |
| **乘法（高64位）** | ❌ 分开 | `SMULH`（有符号）/ `UMULH`（无符号） |
| **除法** | ❌ 分开 | `SDIV`（向零截断）/ `UDIV`，除零不报错 |

> **一句话**：加减乘（低位）不需要区分有符号无符号；除法必须选。

---

## HFT 实战视角

### 1. ADDS/SUBS 省掉 CMP（循环优化）

```asm
; ❌ 低效：subs + cmp 两条指令
loop:
    subs x0, x0, #1        ; 计数器减 1
    cmp  x0, #0            ; 多余！subs 已经设置了 Z 标志
    b.gt loop

; ✅ 高效：subs 直接跟条件分支
loop:
    subs x0, x0, #1        ; 减 1 同时设置 NZCV
    b.gt loop              ; 直接用 subs 设置的标志，省一条指令
```

> 减少一条指令 = 减少一个流水线周期。在 HFT 热路径中，循环计数器的优化累积效果显著。

### 2. MADD 优先（价格计算）

```asm
; 总价 = 基础费 + 单价 × 数量
; x1 = price, x2 = qty, x3 = base_fee
madd x0, x1, x2, x3       ; x0 = base_fee + price * qty
                           ; 一条指令完成，比 mul + add 少一条

; 净额 = 总额 - 手续费率 × 数量
; x1 = fee_rate, x2 = qty, x3 = total
msub x0, x1, x2, x3       ; x0 = total - fee_rate * qty
```

```c
// C 代码对应
// 编译器会自动把 a + b * c 优化为 MADD
int64_t total = base_fee + price * qty;  // → madd
int64_t net   = total - fee_rate * qty;  // → msub
```

### 3. SDIV 精度陷阱（百分比计算）

```c
// HFT 价格变动百分比计算
int64_t price_change = new_price - old_price;   // 可能为负
int64_t result = price_change / old_price;       // SDIV 向零截断

// ⚠️ 陷阱：-7 / 2 = -3，不是 -4
// 做净值/百分比计算时，向零截断会带来系统性偏差
// 如果需要向下取整，要用额外逻辑：
int64_t floored_div(int64_t a, int64_t b) {
    int64_t q = a / b;
    int64_t r = a % b;
    if ((r != 0) && ((r < 0) != (b < 0)))
        q--;                    // 向下取整修正
    return q;
}
```

### 4. 除零安全检查

```c
// ARM64 除零不会 crash，但会静默返回 0
// HFT 代码必须校验，否则会导致错误的交易决策

inline int64_t safe_pct(int64_t num, int64_t denom) {
    if (denom == 0) {
        // 不能静默返回 0，要记录告警
        // log_warn("division by zero: num=%ld", num);
        return 0;
    }
    return num / denom;
}
```

### 5. 移位替代乘除

```asm
; 乘除 2 的幂次用移位替代，零延迟（在 ALU 移位器中完成）
lsl x0, x0, #3            ; x0 *= 8  （替代 mul x0, x0, #8）
asr x0, x0, #2            ; x0 /= 4  （有符号，替代 sdiv）
                          ; 详见 §4.3 移位
```

> 详见 [§4.3 移位](03-shift.md)

---

## 自测题

1. `CMP x0, x1` 等价于什么指令？
<details><summary>答案</summary>

`SUBS XZR, x0, x1`——减法结果丢弃（写入 XZR），只设置 NZCV 标志。

</details>

2. `ADD` 和 `ADDS` 的区别是什么？
<details><summary>答案</summary>

ADD 只做加法，不修改 NZCV。ADDS 做加法并设置 NZCV 标志（N=结果为负、Z=结果为零、C=无符号进位、V=有符号溢出）。

</details>

3. 如何用 ADC 实现 128 位加法？
<details><summary>答案</summary>

```asm
adds x0, x0, x2    ; 低 64 位加，设置 C 标志
adc  x1, x1, x3    ; 高 64 位加 + 进位
```

先 ADDS 设置进位标志，再 ADC 把进位加到高位。

</details>

4. `MUL` 取低 64 位时，有符号和无符号乘法结果相同吗？为什么？
<details><summary>答案</summary>

**相同**。这是补码的性质：两个 N 位补码数相乘，结果的低 N 位与无符号乘法的低 N 位完全一致。只有高位（第 N+1~2N 位）才因符号扩展而不同。

</details>

5. `SDIV` 和 `UDIV` 有什么区别？`-7 / 2` 分别得多少？
<details><summary>答案</summary>

- `SDIV`：有符号除法，向零截断。`-7 / 2 = -3`（不是 -4，向零截断丢小数）
- `UDIV`：无符号除法。同样的比特流 `0xFFFF...FFF9 / 2` = 极大正数

**除法是加减乘中唯一必须显式选择有符号/无符号的运算。**

</details>

6. ARM64 除零会产生异常吗？
<details><summary>答案</summary>

**不会**。ARM64 除零结果为 0，不产生异常也不 trap。软件必须自行检查除数是否为 0。这与 x86 不同（x86 除零触发 #DE 异常）。

</details>

7. `MADD x0, x1, x2, x3` 计算什么？如果不需要加数怎么写？
<details><summary>答案</summary>

`x0 = x3 + x1 * x2`（乘加）。如果不需要加数，用 `XZR` 代替：`mul x0, x1, x2` ≡ `madd x0, x1, x2, xzr`。

</details>

8. 补码 `-5` 在 64 位寄存器中的十六进制值是什么？
<details><summary>答案</summary>

`0xFFFFFFFFFFFFFFFB`。计算过程：`~5 + 1 = ~0x0000000000000005 + 1 = 0xFFFFFFFFFFFFFFFA + 1 = 0xFFFFFFFFFFFFFFFB`。

</details>

---

## 自测速查（快速复习）

| 问题 | 速答 |
|------|------|
| CMP x0,x1 等价 | `SUBS XZR, X0, X1` |
| ADD vs ADDS | ADD 只算结果；ADDS 同时设置 NZCV |
| 128 位加法 | `adds` 低 64 位 → `adc` 高 64 位带进位 |
| MUL 低 64 位有/无符号 | 相同（补码数学性质） |
| SDIV 截断方式 | 向零截断，-7/2=-3 |
| UDIV 含义 | 把比特流当无符号大数做除法 |
| ARM64 除零行为 | 不异常，返回 0 |
| madd x0,x1,x2,x3 | x0 = x3 + x1 × x2 |
| 64 位 -5 补码 | `0xFFFFFFFFFFFFFFFB` |
| 两处 S 区别 | 算术 S = Set flags；加载 S = Sign 扩展 |

---

## 参考与延伸

- 原书 §4.1
- [4.2 NZCV 标志](02-nzcv.md)
- [补码/有符号无符号专篇](../../SIGNED-UNSIGNED.md)
- [S 后缀辨析](../../S-SUFFIX.md)
- [5.2 条件选择指令](../../chapter-05-a64-compare-branch/notes/section-0-本章完整概述.md)
