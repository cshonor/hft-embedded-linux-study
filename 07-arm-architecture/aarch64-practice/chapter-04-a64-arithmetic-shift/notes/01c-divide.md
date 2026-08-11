# 4.1c 算术指令 — 除法与取负

> 来源：§4.1 · 精读 · [← 乘法](01b-multiply.md) · [加减法](01-arithmetic.md) · [章总览](section-0-本章完整概述.md)

## 本节讲什么

SDIV / UDIV 除法指令、取负指令、补码运算总结表。
乘法见 [01b-multiply.md](01b-multiply.md)，加减法见 [01-arithmetic.md](01-arithmetic.md)。

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

### SDIV 精度陷阱（百分比计算）

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

### 除零安全检查

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

---

## 自测题

1. `SDIV` 和 `UDIV` 有什么区别？`-7 / 2` 分别得多少？
<details><summary>答案</summary>

- `SDIV`：有符号除法，向零截断。`-7 / 2 = -3`（不是 -4，向零截断丢小数）
- `UDIV`：无符号除法。同样的比特流 `0xFFFF...FFF9 / 2` = 极大正数

**除法是加减乘中唯一必须显式选择有符号/无符号的运算。**

</details>

2. ARM64 除零会产生异常吗？
<details><summary>答案</summary>

**不会**。ARM64 除零结果为 0，不产生异常也不 trap。软件必须自行检查除数是否为 0。这与 x86 不同（x86 除零触发 #DE 异常）。

</details>

3. `NEG x0, x1` 等价于哪条指令？`NEGS` 呢？
<details><summary>答案</summary>

`NEG x0, x1` ≡ `SUB x0, XZR, x1`（零寄存器减去 x1）。
`NEGS` 同理 ≡ `SUBS x0, XZR, x1`，额外更新 NZCV 标志。

</details>

---

## 自测速查（快速复习）

| 问题 | 速答 |
|------|------|
| SDIV 截断方式 | 向零截断，-7/2=-3 |
| UDIV 含义 | 把比特流当无符号大数做除法 |
| ARM64 除零行为 | 不异常，返回 0 |
| NEG 等价 | `SUB x0, XZR, x1` |
| 除法有/无符号 | 必须分开（SDIV/UDIV） |

---

## 参考与延伸

- 原书 §4.1
- [← 乘法指令](01b-multiply.md)
- [加减法指令](01-arithmetic.md)
- [4.2 NZCV 标志](02-nzcv.md)
- [补码/有符号无符号专篇](../../SIGNED-UNSIGNED.md)
