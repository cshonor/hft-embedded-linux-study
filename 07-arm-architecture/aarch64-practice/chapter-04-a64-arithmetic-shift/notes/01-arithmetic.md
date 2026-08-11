# 4.1 算术指令 — 加减法

> 来源：§4.1 · 精读 · [章总览](section-0-本章完整概述.md) · [补码专篇](../../SIGNED-UNSIGNED.md) · [乘法 →](01b-multiply.md) · [除法 →](01c-divide.md)

## 本节讲什么

ADD/SUB/CMP/CMN/ADC/SBC 等加减法指令，以及补码与加减运算的关系。
乘法见 [01b-multiply.md](01b-multiply.md)，除法见 [01c-divide.md](01c-divide.md)。

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

### 什么时候用 ADD vs ADDS？（新手选择指南）

> **一句话：后面有 B.cond 条件跳转 → 用 ADDS；没有 → 用 ADD。**

| 指令 | 做什么 | 什么时候用 |
|------|--------|-----------|
| **ADD** | 只算结果，**不碰** NZCV | 后面不需要条件跳转 |
| **ADDS** | 算结果 + **更新** NZCV | 后面要用 B.EQ/B.NE/B.LT 等条件跳转 |
| **CMN** | 只更新 NZCV，结果丢弃 | 只想判断 x0+x1 是否为 0，不想要结果 |

**场景 1：纯粹算数，不用判断 → 用 ADD**

```asm
; 计算地址偏移，不需要判断
add x0, x1, x2        ; x0 = x1 + x2，算完直接用

; 计算数组基地址（下标 * 8 + 基址）
add x4, x0, x1, lsl #3  ; x4 = x0 + x1*8

; 栈指针调整
add sp, sp, #16        ; 弹栈 16 字节，不需要判断
```

**场景 2：算完要判断 → 用 ADDS**

```asm
; 计数器递增，达到上限要跳转
adds x0, x0, #1        ; x0++，同时更新 NZCV
b.eq done              ; x0 == 0 了？跳出循环

; 检查加法是否溢出
adds x0, x1, x2
b.vs overflow_handler  ; V=1 说明有符号溢出，跳去处理
```

**场景 3：只需要判断，不需要结果 → 用 CMN**

```asm
; 只想知道 x0 + x1 是否为 0
cmn x0, x1             ; = adds xzr, x0, x1，结果丢弃，只要 NZCV
b.eq zero_case         ; x0 + x1 == 0 就跳
```

**速记决策树：**

```
后面有 B.cond 跳转吗？
    ├─ 没有 → ADD（省事，不管标志位）
    └─ 有    → ADDS（需要标志位来跳转）
              └─ 不想要结果？→ CMN（= ADDS xzr, ...）
```

> **为什么区分？** ADDS 比 ADD 多做一件事（写 NZCV），虽然差异极小，但在高频循环里能省就省。编译器会自动帮你选——后面没跟条件跳转就生成 ADD，跟了就生成 ADDS。同理 SUB/SUBS、NEG/NEGS 一样：**后面要条件跳转加 S，不要就不加。**

### 逐行解释

**加法组**

| 指令 | 做什么 | 改标志？ |
|------|--------|---------|
| `ADD Rd, Rn, Op2` | `Rd = Rn + Op2` | ❌ 只算数值 |
| `ADDS Rd, Rn, Op2` | 同上 | ✅ 产生进位写入 C，用于多字节大数运算 |
| `ADC Rd, Rn, Op2` | `Rd = Rn + Op2 + C`，把上一次运算的进位 C 一起加进来 | ❌ |
| `ADCS Rd, Rn, Op2` | 同 ADC | ✅ |

> **128 位加法标准写法**：先用 `ADDS` 做低 64 位，自动产出进位 C；再用 `ADC` 做高 64 位，把 C 加进去。

**减法组**

| 指令 | 做什么 | 改标志？ |
|------|--------|---------|
| `SUB Rd, Rn, Op2` | `Rd = Rn − Op2` | ❌ |
| `SUBS Rd, Rn, Op2` | 同上 | ✅ CMP 本质就是 `SUBS XZR, Rn, Op2` |
| `SBC Rd, Rn, Op2` | `Rd = Rn − Op2 − !C`（`NOT(C)` 取反进位） | ❌ |
| `SBCS Rd, Rn, Op2` | 同 SBC | ✅ |

> **SBC 的 !C 是什么意思？** ARM 减法中 C = NOT(借位)。够减时 C=1（无借位），不够减时 C=0（有借位）。SBC 减去 `!C`：当不够减（C=0）时 `!C=1`，多减 1（借位）；够减（C=1）时 `!C=0`，不多减。这样 128 位减法的高位就能正确处理借位。

**取负组**

| 指令 | 做什么 | 改标志？ |
|------|--------|---------|
| `NEG Rd, Op2` | `Rd = −Op2`，伪指令，汇编器翻译成 `SUB Rd, XZR, Op2` | ❌ |
| `NEGS Rd, Op2` | 同上 | ✅ |
| `NGC Rd, Op2` | `Rd = −Op2 − !C`，带借位取负 | ❌ |

> NGC 用于 128 位大数取负，日常几乎见不到。

**比较组（伪指令）**

| 指令 | 等价于 | 用途 |
|------|--------|------|
| `CMP Rn, Op2` | `SUBS XZR, Rn, Op2` | 做减法，结果丢给 XZR（丢弃），只修改 NZCV，给后面 `b.eq/b.lt/b.lo` 用 |
| `CMN Rn, Op2` | `ADDS XZR, Rn, Op2` | 执行加法，只更新标志，用来判断 `Rn == −Op2` |

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

## 易错坑

| # | 错误写法 | 问题 | 正确做法 |
|---|---------|------|---------|
| 1 | `add x0, x0, #1` 然后 `b.ne loop` | ADD 不带 S，NZCV 根本没变，b.ne 判断的是**旧的标志** | 用 `adds x0, x0, #1` 或显式 `cmp` |
| 2 | `adc x1, x1, x3` 前面没有 `adds` | ADC 依赖上一条 ADDS 产生的 C 进位标志，顺序不能乱 | 必须先 `adds` 低 64 位，再 `adc` 高 64 位 |
| 3 | `sbc x1, x1, x3` 前面没有 `subs` | SBC 依赖上一条 SUBS 产生的 C 借位标志 | 必须先 `subs` 低 64 位，再 `sbc` 高 64 位 |
| 4 | 把 `ADDS` 的 S 当成 Signed | S = Set flags，不是有符号 | 有符号/无符号共用 ADD/SUB，靠 NZCV 区分 |

```asm
; ❌ 典型 bug：ADD 后直接条件跳转
    add  x0, x0, #1      ; NZCV 没变！
    b.eq done            ; 判断的是旧标志 → 逻辑错误

; ✅ 正确：用 ADDS
    adds x0, x0, #1      ; NZCV 更新
    b.eq done            ; 判断的是本次加法的结果
```

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

### 2. 移位替代乘除

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

4. 补码 `-5` 在 64 位寄存器中的十六进制值是什么？
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
| 两处 S 区别 | 算术 S = Set flags；加载 S = Sign 扩展 |

---

## 参考与延伸

- 原书 §4.1
- [乘法指令 →](01b-multiply.md) · [除法指令 →](01c-divide.md)
- [4.2 NZCV 标志](02-nzcv.md)
- [补码/有符号无符号专篇](../../SIGNED-UNSIGNED.md)
- [S 后缀辨析](../../S-SUFFIX.md)
