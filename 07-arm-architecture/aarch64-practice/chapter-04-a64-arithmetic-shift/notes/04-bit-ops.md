# 4.4 位操作指令

> 来源：§4.4 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

AND/ORR/EOR/BIC 等位操作指令，立即数编码挑战，以及 MOV 系列伪指令。

---

## 位操作指令总表

| 指令 | 作用 | C 等价 | 改 NZCV？ |
|------|------|--------|----------|
| `AND` | 按位与 | `a & b` | ❌ |
| `ANDS` | 按位与 + 设标志 | `a & b` | ✅ |
| `ORR` | 按位或 | `a \| b` | ❌ |
| `EOR` | 按位异或 | `a ^ b` | ❌ |
| `BIC` | 位清除（AND NOT） | `a & ~b` | ❌ |
| `BICS` | 同上 + 设标志 | `a & ~b` | ✅ |
| `TST` | 测试（≡ ANDS XZR） | `a & b`（丢弃） | ✅ |
| `EON` | 异或非 | `a ^ ~b` | ❌ |
| `ORN` | 或非 | `a \| ~b` | ❌ |

---

## 常见用法

### 置位、清位、翻转、提取

```asm
; 置位第 4 位（置 1）
orr  x0, x0, #(1 << 4)     ; x0 |= (1 << 4)

; 清除第 4 位（置 0）
bic  x0, x0, #(1 << 4)     ; x0 &= ~(1 << 4)

; 翻转第 4 位
eor  x0, x0, #(1 << 4)     ; x0 ^= (1 << 4)

; 测试第 4 位
tst  x0, #(1 << 4)         ; 等价于 ands xzr, x0, #(1 << 4)
                           ; 若 bit4=0 → Z=1; bit4=1 → Z=0

; 提取低 8 位
and  x0, x1, #0xFF         ; x0 = x1 & 0xFF
```

### 位操作真值表

```
 A | B | A&B | A|B | A^B | A & ~B (BIC)
---|---|-----|-----|-----|-----------
 0 | 0 |  0  |  0  |  0  |   0
 0 | 1 |  0  |  1  |  1  |   0
 1 | 0 |  0  |  1  |  1  |   1
 1 | 1 |  1  |  1  |  0  |   0
```

---

## BIC 详解

`BIC`（Bit Clear）= `Rd = Rn & (~Op2)`：

```asm
bic  x0, x0, #0xFF         ; 清除 x0 的低 8 位，其余不变
bic  x0, x0, #(1 << 7)     ; 清除 x0 的第 7 位
bic  x0, x0, #0xF          ; 清除低 4 位 = 向下取整到 16 的倍数
```

> ⚠️ **BIC 不是"整寄存器取反"**。它是对**操作数取反后再 AND**，即"清掉指定位置 1 的那些位"。

### BIC 用法对比

```asm
; 清除第 5 位 — BIC 更直观
bic  x0, x0, #(1 << 5)     ; 一条指令

; 等价写法 — 用 AND + 立即数掩码（需要手动算取反值）
and  x0, x0, #0xFFFFFFFFFFFFFFDF   ; ~(1<<5) = 0xFF...DF，立即数很长不直观
```

---

## EOR 的特殊用途

```asm
; 1. 翻转特定位（toggle）
eor  x0, x0, #(1 << 4)     ; 第 4 位：0→1, 1→0

; 2. 清零自身
eor  x0, x0, x0            ; x0 = 0 ≡ mov x0, xzr

; 3. 简单加密/解密（异或密钥）
eor  x0, x0, x1            ; 加密
eor  x0, x0, x1            ; 再异或一次 = 解密（还原）

; 4. 无临时变量交换两个寄存器
eor  x0, x0, x1            ; x0 = x0 ^ x1
eor  x1, x1, x0            ; x1 = x1 ^ (x0^x1) = x0
eor  x0, x0, x1            ; x0 = (x0^x1) ^ x0 = x1
; 现在 x0 和 x1 互换了
```

> 注意：无临时变量交换虽然炫技，但在实际代码中不推荐——影响流水线和寄存器分配，不如用临时变量。

---

## 立即数编码挑战

ARM64 位操作立即数**不是任意 64 位值**，而是用 `N:immr:imms` 编码的"位段重复"模式：

| 规则 | 说明 |
|------|------|
| 编码方式 | 13 位字段（1位N + 6位immr + 6位imms）描述一个位模式 |
| 可表示的值 | 连续的 1/0 模式在 64 位内重复（如 `0xFF`, `0xFFF`, `0x5555555555555555` 等） |
| 不能表示的值 | 如 `0x12345678`（非连续模式）→ 需要用 `MOVZ`/`MOVK` 组合 |

```asm
; ✅ 合法立即数（连续位段模式）
and  x0, x1, #0xFF         ; 低 8 位掩码
and  x0, x1, #0xFFFF       ; 低 16 位掩码
orr  x0, x0, #0x80000000   ; 最高位（bit 63:32 重复模式）

; ❌ 非法立即数 → 编译器报错或拆成多条指令
; and  x0, x1, #0x12345678  ← 无法直接编码
; 解决方案：
movz x2, #0x5678
movk x2, #0x1234, lsl #16
and  x0, x1, x2            ; 用寄存器代替
```

> 经验法则：掩码是连续的 1（如 `0xFF`, `0xFFF`, `0xFFFFFF`）或重复模式（如 `0x5555...`），一般可以直接编码。

---

## MOV 系列伪指令

ARM64 没有 `MOV` 真指令，它是伪指令，汇编器根据操作数选 `MOVZ`/`MOVK`/`ORR` 等：

| 伪指令 | 展开为 | 用途 |
|--------|--------|------|
| `MOV x0, x1` | `ORR x0, xzr, x1` | 寄存器间搬运 |
| `MOV x0, #imm` | `MOVZ`/`MOVK` 组合 | 立即数加载 |
| `MOVZ x0, #imm, lsl #n` | 直接编码 | 16 位立即数 + 左移 |
| `MOVK x0, #imm, lsl #n` | 保持其他位不变 | 修改某 16 位段 |
| `MOVN x0, #imm, lsl #n` | `x0 = NOT(imm << n)` | 取反移动 |

### 构造 64 位立即数

```asm
; 构造 0x1234567890ABCDEF
movz x0, #0xCDEF              ; x0 = 0x000000000000CDEF
movk x0, #0x90AB, lsl #16     ; x0 = 0x0000000090ABCDEF
movk x0, #0x5678, lsl #32     ; x0 = 0x0000567890ABCDEF
movk x0, #0x1234, lsl #48     ; x0 = 0x1234567890ABCDEF

; 构造 -1（全 1）
movn x0, #0                   ; x0 = ~0 = 0xFFFFFFFFFFFFFFFF
```

> `MOVZ` = Move with Zero（其他位补 0），`MOVK` = Move with Keep（其他位不变），`MOVN` = Move with NOT（取反）。

### MOVN 构造常用掩码

```asm
movn x0, #0             ; x0 = 0xFFFFFFFFFFFFFFFF (=-1)
movn x0, #0xFF          ; x0 = 0xFFFFFFFFFFFFFF00
movn x0, #0xFFFF, lsl #16 ; x0 = 0xFFFFFFFF0000FFFF
```

---

## TST 详解

```asm
tst  x0, #0xF            ; 等价于 ands xzr, x0, #0xF
                          ; 结果丢弃，只设 NZCV
                          ; 若 x0 低 4 位全 0 → Z=1
                          ; 若 x0 低 4 位非全 0 → Z=0
```

判断方法：
```asm
tst  x0, #(1 << 7)      ; 测试第 7 位
b.eq bit_is_zero         ; Z=1 → 第7位=0
; Z=0 → 第7位=1
```

> `TST` 和 `CMP` 类似：都丢弃结果只设标志。`CMP` ≡ `SUBS XZR`，`TST` ≡ `ANDS XZR`。

---

## HFT 关联

位操作在协议解析和标志管理中至关重要：
- 市场数据协议的位域标志用 AND/TST 提取
- 订单状态标志用 ORR/BIC 设置/清除
- EOR 翻转位用于状态切换（如开关中断屏蔽）
- BIC 比 `AND NOT` 更简洁，一条指令完成位清除
- `MOVZ`/`MOVK` 组合构造 64 位常量（如地址掩码）

---

## 自测题

1. 如何清除 x0 的第 7 位而不影响其他位？
<details><summary>答案</summary>

```asm
bic  x0, x0, #(1 << 7)
```

BIC = AND NOT，清除指定位置 1 的那些位。或用 AND 加取反掩码：`and x0, x0, #0xFFFFFFFFFFFFFF7F`（BIC 更直观）。

</details>

2. `TST x0, #0xF` 执行后如何判断 x0 低 4 位是否全为 0？
<details><summary>答案</summary>

TST 等价于 ANDS XZR。如果 x0 低 4 位全 0，AND 结果为 0 → Z=1。用 `B.EQ` 判断 Z=1 即可。

</details>

3. EOR 有什么特殊用途？
<details><summary>答案</summary>

1. 翻转特定位（toggle）：`eor x0, x0, #mask`
2. 清零：`eor x0, x0, x0` → x0=0
3. 简单加密：异或密钥可加密/解密（同一密钥异或两次还原）
4. 无临时变量交换：`eor x0,x0,x1; eor x1,x1,x0; eor x0,x0,x1`

</details>

4. 如何用 MOVZ/MOVK 构造 `0x1234567890ABCDEF`？
<details><summary>答案</summary>

```asm
movz x0, #0xCDEF
movk x0, #0x90AB, lsl #16
movk x0, #0x5678, lsl #32
movk x0, #0x1234, lsl #48
```

每次 MOVZ/MOVK 操作 16 位，其他位 MOVZ 补 0，MOVK 保持不变。

</details>

5. `AND x0, x1, #0x12345678` 能编译通过吗？如果不能怎么办？
<details><summary>答案</summary>

**不能**。`0x12345678` 不是连续位段模式，无法用 ARM64 的 13 位立即数编码表示。解决方案：先用 MOVZ/MOVK 把值放入寄存器，再用 `AND x0, x1, x2`。

</details>

6. `MOV x0, #-1` 汇编器会展开成什么？
<details><summary>答案</summary>

展开为 `MOVN x0, #0`（取反零 = 全 1 = -1）。也可以展开为 `ORN x0, xzr, xzr`（与零或取反），但 MOVN 更常见。

</details>

---

## 参考与延伸

- 原书 §4.4
- [4.3 移位指令](03-shift.md)
- [4.5 位段提取](05-bit-field.md)
- [4.6 典型例子](06-examples.md)
- ARM ARM §C3.4
