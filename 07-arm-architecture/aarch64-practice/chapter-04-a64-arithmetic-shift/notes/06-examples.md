# 4.6 典型例子

> 来源：§4.6 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

算术/移位/位操作的综合应用案例——把前面的指令串起来用。

---

## 1. 对齐计算

### 向上对齐到 16 字节边界

```asm
; 方法：加 15 再清低 4 位
add  x0, x0, #15           ; 先 +15 确保越过边界
bic  x0, x0, #0xF          ; 清除低 4 位 = 向下取整到 16 的倍数
```

```
举例：x0 = 0x1003
  add x0, x0, #15 → x0 = 0x1012
  bic x0, x0, #0xF → x0 = 0x1000  ← 已对齐到 16

举例：x0 = 0x1000 (已对齐)
  add x0, x0, #15 → x0 = 0x100F
  bic x0, x0, #0xF → x0 = 0x1000  ← 仍是 0x1000
```

> 注意：已对齐的地址经过 +15 再清低位会回到自身，因为 `bic` 是向下取整。

### 向下对齐到 8 字节边界

```asm
bic  x0, x0, #7            ; 清除低 3 位 = 向下取整到 8 的倍数
                           ; 7 = 0b111，清低 3 位
```

---

## 2. 位域打包

### 构造订单消息头

```asm
; 将 type(x1, 4bit) 和 flags(x2, 8bit) 打包到 x0
mov  x0, xzr               ; 清零
bfi  x0, x1, #0, #4        ; type → x0[3:0]
bfi  x0, x2, #4, #8        ; flags → x0[11:4]
```

```
打包后 x0:
  bit 11  10  9  8  7  6  5  4 | 3  2  1  0
  ├──┤ flags (8 bit)           ├──┤ type (4 bit)
```

### 打包 + 立即数组合

```asm
; 构造 32 位 IPv4 头部：version(4) + IHL(4) + DSCP(6) + ECN(2) + ...
mov  w0, #(4 << 12)         ; version=4 在 bit[15:12]
bfi  w0, w1, #8, #4         ; IHL → bit[11:8]
bfi  w0, w2, #2, #6         ; DSCP → bit[7:2]
bfi  w0, w3, #0, #2         ; ECN → bit[1:0]
```

---

## 3. 绝对值（无分支）

```asm
; x0 = |x1| — 利用 ASR #63 提取符号位
asr  x2, x1, #63            ; 正数→x2=0; 负数→x2=-1(全1)
eor  x0, x1, x2             ; 正数→x0=x1^0=x1; 负数→x0=x1^(-1)=~x1
sub  x0, x0, x2             ; 正数→x0=x1-0=x1; 负数→x0=~x1+1=-x1
```

### 用 CNEG（条件取反，更简洁）

```asm
; x0 = |x1| — 使用 CNEG
subs xzr, x1, #0            ; 设置 NZCV（x1 vs 0）
cneg x0, x1, mi             ; 如果 N=1(负数) → x0 = -x1; 否则 x0 = x1
```

> CNEG 是条件取反指令（见 [§5.2 条件选择指令](../../chapter-05-a64-compare-branch/notes/section-0-本章完整概述.md)）。单条指令完成条件取反，不需要分支。

### 原理拆解

| 步骤 | 正数 x1=5 | 负数 x1=-5 |
|------|----------|-----------|
| `asr x2, x1, #63` | x2=0 | x2=-1（全1） |
| `eor x0, x1, x2` | x0=5^0=5 | x0=(-5)^(-1)=~(-5)=4 |
| `sub x0, x0, x2` | x0=5-0=5 | x0=4-(-1)=5 |

> 负数取绝对值 = 补码取负 = 取反加一。`xor(-1)` = 取反，`-(-1)` = +1。

---

## 4. 乘法替代

### 用 LSL 替代乘法

```asm
; x0 = x1 * 8
lsl  x0, x1, #3            ; 1 cycle

; 对比：
mul  x0, x1, #8            ; 不存在！MUL 不支持立即数
mov  x2, #8
mul  x0, x1, x2            ; 3-5 cycle + 额外寄存器
```

### 用 MADD 做乘加

```asm
; 总价 = 基础费 + 单价 × 数量
; x0 = fee + price * qty
madd x0, x1, x2, x3        ; x0 = x3 + x1 * x2
                           ; x1=price, x2=qty, x3=fee
```

### 移位替代除法

```asm
; x0 = x1 / 4（无符号）
lsr  x0, x1, #2            ; 1 cycle

; x0 = x1 / 4（有符号补码）
asr  x0, x1, #2            ; 1 cycle，保持符号

; 对比：
mov  x2, #4
udiv x0, x1, x2            ; 20+ cycle！
```

> 移位除法只适用于 2 的幂次。非 2 的幂需要用除法指令或乘法+缩放。

---

## 5. 有符号最大值（无分支）

```asm
; x0 = max(x1, x2) — 有符号比较
cmp  x1, x2
csel x0, x1, x2, ge        ; 如果 GE(有符号≥) → x0=x1; 否则 x0=x2
```

> CSEL = Conditional Select，根据 NZCV 选择结果（见 [§5.2](../../chapter-05-a64-compare-branch/notes/section-0-本章完整概述.md)）。无分支，避免分支预测失败。

---

## 6. 向上取整到 2 的幂

```asm
; x0 = round_up(x1, 8)  — 向上取整到 8 的倍数
add  x0, x1, #7
bic  x0, x0, #7            ; 清低 3 位

; 通用版本：round_up(x1, n)，n 是 2 的幂
; x2 = n - 1
add  x0, x1, x2
bic  x0, x0, x2            ; 清低 log2(n) 位
```

---

## HFT 关联

这些模式在 HFT 代码中高频出现：
- 对齐计算：DMA buffer 地址对齐 → `bic x0, x0, #0xF`
- 位域打包：构造订单消息头 → BFI 一步插入字段
- 绝对值：计算价格变动 abs(new - old) → CNEG 条件取反无分支
- 无分支代码消除分支预测失败 → 减少流水线气泡
- 乘法替代：`lsl #3` 替代 `*8`，`asr #2` 替代 `/4`
- 乘加：`madd` 一条指令完成 `base + price * qty`

---

## 自测题

1. 如何将 x0 向下对齐到 8 字节边界（一条指令）？
<details><summary>答案</summary>

`bic x0, x0, #7`。清除低 3 位等价于向下取整到 8 的倍数。

</details>

2. CNEG x0, x1, mi 的作用是什么？比 CMP+NEG 有什么优势？
<details><summary>答案</summary>

CNEG 是条件取反：如果 mi(N=1, 负数)条件成立则 x0 = -x1，否则 x0 = x1。优势：单条指令完成条件取反，不需要分支，避免分支预测失败。

</details>

3. 写出提取 x0 bit[27:20] 并符号扩展到 x1 的指令。
<details><summary>答案</summary>

```asm
sbfx x1, x0, #20, #8
```

SBFX 自动符号扩展。如果是有符号字段必须用 SBFX 而非 UBFX。

</details>

4. `madd x0, x1, x2, x3` 计算什么？HFT 场景如何用？
<details><summary>答案</summary>

`x0 = x3 + x1 * x2`（乘加）。HFT 场景：`总价 = 基础费 + 单价 × 数量`，一条指令完成。

</details>

5. 用移位实现 `x0 = x1 * 16`（无符号和有符号都适用）。
<details><summary>答案</summary>

```asm
lsl x0, x1, #4
```

LSL #4 = 左移 4 位 = × 16。左移补 0 对补码有符号和无符号都正确。

</details>

6. 如何用 ASR/EOR/SUB 三条指令实现 `x0 = |x1|`（无分支绝对值）？
<details><summary>答案</summary>

```asm
asr  x2, x1, #63    ; 提取符号位：正→0，负→-1(全1)
eor  x0, x1, x2     ; 正→x1^0=x1，负→x1^(-1)=~x1
sub  x0, x0, x2     ; 正→x1-0=x1，负→~x1-(-1)=~x1+1=-x1
```

原理：负数取绝对值 = 取反加一 = 补码取负。

</details>

---

## 参考与延伸

- 原书 §4.6
- [4.1 算术指令](01-arithmetic.md)
- [4.5 位段提取](05-bit-field.md)
- [5.2 条件选择指令](../../chapter-05-a64-compare-branch/notes/section-0-本章完整概述.md)
