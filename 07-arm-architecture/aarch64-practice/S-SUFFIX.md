# `S` 后缀 ≠ 有符号数

> 新手最大坑之一 · 配套 [NZCV.md](./NZCV.md) · [SIGNED-UNSIGNED.md](./SIGNED-UNSIGNED.md) · [Ch4](./chapter-04-a64-arithmetic-shift/notes/section-0-本章完整概述.md) · [Ch3](./chapter-03-a64-load-store/notes/section-0-本章完整概述.md)

---

## 重点结论

**算术指令末尾的 `S` ≠ signed（有符号）。**  
真实含义：**Set flags** —— 把运算结果写入 PSTATE 的 **NZCV**。  
跟操作数是有符号还是无符号 **没有绑定关系**。

---

## 1. `S` 到底干什么

| 指令 | 行为 |
|------|------|
| `ADD` | 加法，**不更新** NZCV |
| `ADDS` | 加法，算完后刷新 **N/Z/C/V** |

`ADDS` 一次运算**同时**产出：

- **C** → 给**无符号**用的进位  
- **V** → 给**有符号补码**用的溢出  

`S` 只是「把标志写出来」；后续用 C 还是 V，是**软件自己选**，不是 `S` 决定数据类型。

```asm
adds x0, x1, x2
; 无论你把 x1/x2 当有符号还是无符号，C 和 V 都会更新
; 无符号后续看 C；有符号补码后续看 N、V
```

---

## 2. 两种完全不一样的「S」

### ① 指令**末尾** S（`ADDS` / `SUBS` / `ANDS` …）

> **Set-flags**：修改 NZCV，**和有符号无关**。

### ② 访存指令名里的 S（`LDRSB` / `LDRSH`）

> **Sign-extend**：符号扩展；**这个 S 才表示按有符号补码扩展**。

```asm
LDRB    ; Load Byte，零扩展（无符号解读）
LDRSB   ; Load Signed Byte，符号扩展（这里的 S = Sign）
```

| 位置 | 例子 | 含义 |
|------|------|------|
| **末尾** S | `ADDS`、`SUBS` | Set flags |
| **中间** S | `LDRSB`、`LDRSH` | Sign-extend |

> 记忆口诀：**末尾 S = 设置标志；中间 S = 符号扩展。**

---

## 3. 例子：同一条 SUBS，两种比较

`CMP` ≡ `SUBS XZR, …`（带末尾 S = Set flags）：

```asm
cmp x0, x1
b.hs  label   ; HS 看 C → 【无符号】大小
b.lt  label   ; LT 看 N^V → 【有符号补码】大小
```

同一条带 S 的减法，后面既能无符号也能有符号判断 → **证明末尾 S 不代表有符号数**。

---

## 极简必背

1. 算术末尾 S（`ADDS`/`SUBS`）：**Set flags，更新 NZCV，≠ 有符号**。  
2. 访存中间 S（`LDRSB`）：**Sign，符号扩展，才是有符号补码语义**。  
3. 带 S 算术同时产 C 与 V；用哪套由**条件后缀**决定，不由 S 决定。
