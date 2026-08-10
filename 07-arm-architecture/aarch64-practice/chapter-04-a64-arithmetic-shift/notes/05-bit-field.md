# 4.5 位段提取与插入

> 来源：§4.5 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

UBFX/SBFX/BFI/BFC 等位段操作指令，在内核页表项解析和协议字段打包中的高频应用。

---

## 位段指令总表

| 指令 | 作用 | 扩展方式 | C 等价 |
|------|------|---------|--------|
| `UBFX` | 从 Rn 的 lsb 起取 width 位 → Rd | **零扩展**（高位补 0） | `(Rn >> lsb) & ((1<<width)-1)` |
| `SBFX` | 同上 | **符号扩展**（高位补符号位） | 有符号版本 |
| `BFI` | 把 Rn 低 width 位写入 Rd 的 lsb 位置 | 不改变其他位 | 位段插入 |
| `BFC` | 把 Rd 的 lsb 起 width 位清零 | 等价于 BFI with XZR | 位段清除 |

> UBFX = Unsigned Bit Field eXtract；SBFX = Signed Bit Field eXtract；BFI = Bit Field Insert；BFC = Bit Field Clear。

---

## 语法格式

```asm
UBFX Rd, Rn, #lsb, #width     ; 从 Rn[lsb+width-1 : lsb] 提取，零扩展到 Rd
SBFX Rd, Rn, #lsb, #width     ; 同上，符号扩展
BFI  Rd, Rn, #lsb, #width     ; 把 Rn[width-1:0] 插入 Rd[lsb+width-1:lsb]
BFC  Rd, #lsb, #width         ; 把 Rd[lsb+width-1:lsb] 清零
```

参数说明：
- **lsb**：起始位号（0~63 for X / 0~31 for W）
- **width**：位段宽度（1~64 for X / 1~32 for W）
- **约束**：`lsb + width ≤ 64`（X 寄存器）或 `≤ 32`（W 寄存器）

---

## UBFX vs SBFX：零扩展 vs 符号扩展

### 二进制可视化

```
源寄存器 Rn = 0x...F8   (bit[7:0] = 11111000)

UBFX x1, x0, #0, #8      ; 提取低 8 位 → 零扩展
  提取: 11111000
  扩展: 0x00000000000000F8   (= 248, 正数)

SBFX x1, x0, #0, #8      ; 提取低 8 位 → 符号扩展
  提取: 11111000
  符号位 = 1 (bit[7] = 1)
  扩展: 0xFFFFFFFFFFFFFFF8   (= -8, 负数)
```

> **同一个比特流 `11111000`**：UBFX 解释为 248，SBFX 解释为 -8。选择哪个取决于这个位段是有符号还是无符号。

### 选择规则

| 位段语义 | 用哪个 | 原因 |
|---------|--------|------|
| 无符号字段（如类型码、索引） | `UBFX` | 高位补 0 |
| 有符号字段（如偏移量） | `SBFX` | 高位补符号位 |
| 不确定 | `UBFX` + 手动判断 | 安全默认 |

---

## 实际应用

### 1. 页表项解析（内核高频）

ARM64 页表项（PTE）的位段布局：

```
PTE 格式（简化）:
63          40 39      12 11    9 8 7 6 5 4 3 2 1 0
┌────────────┬──────────┬──────┬──┬─┬─┬─┬─┬─┬─┬─┬─┐
│  PA[47:12] │ reserved │ Attr │AP│Xn│P│NS│SH│AF│nG│V│
└────────────┴──────────┴──────┴──┴─┴─┴─┴─┴─┴─┴─┴─┘
```

```asm
; 提取物理页号（PA[47:12]，bits[47:12]，36位宽）
ubfx x1, x0, #12, #36      ; x1 = PTE[47:12] → 物理页帧号

; 提取属性索引（AttrIndx，bits[4:2]，3位宽）
ubfx x2, x0, #2, #3        ; x2 = PTE[4:2] → 内存属性索引

; 测试有效位（bit 0）
tst  x0, #1                ; V=1 → 页表项有效
```

### 2. 协议字段打包（网络/金融协议）

```asm
; 将 type(x1, 4bit) 和 flags(x2, 8bit) 打包到 x0
mov  x0, xzr                ; 清零
bfi  x0, x1, #0, #4         ; type → x0[3:0]
bfi  x0, x2, #4, #8         ; flags → x0[11:4]

; 结果: x0 = (flags << 4) | (type & 0xF)
; 等价 C: x0 = ((u32)type & 0xF) | (((u32)flags & 0xFF) << 4)
```

```
打包后 x0 布局:
  bit 11  10  9  8  7  6  5  4 | 3  2  1  0
  ├──┤flags (8 bit)            ├──┤type (4 bit)
```

### 3. 绝对值（无分支）

```asm
; x0 = |x1| — 利用 ASR #63 提取符号位做掩码
asr  x2, x1, #63            ; 正数→x2=0; 负数→x2=-1(全1)
eor  x0, x1, x2             ; 正数→x0=x1^0=x1; 负数→x0=x1^(-1)=~x1
sub  x0, x0, x2             ; 正数→x0=x1-0=x1; 负数→x0=~x1-(-1)=~x1+1=-x1
```

> 这个技巧的原理：负数取绝对值 = 取反加一 = 补码取负。`x ^ mask - mask` 在 mask=0（正数）时不变，mask=-1（负数）时等于取反加一。

### 4. 清除位段后重新设置

```asm
; 清除 x0 的 bit[11:4]（8位），然后插入新值 x1 的低 8 位
bfc  x0, #4, #8             ; 清零 x0[11:4]
bfi  x0, x1, #4, #8         ; 插入 x1[7:0] → x0[11:4]
```

> `BFC` 等价于 `BFI Rd, XZR, #lsb, #width`。

### 5. 提取有符号偏移量

```asm
; 指令编码中的 19 位有符号偏移（如 B.cond 的跳转偏移）
; 偏移在 bits[23:5]，19位宽，有符号
sbfx x1, x0, #5, #19        ; 提取并符号扩展 → x1 = 有符号偏移量
```

---

## UBFX vs "LSR + AND" 两条指令

```asm
; 提取 x0 的 bit[23:16]（8 位）

; 方式 1：UBFX（1 条指令）
ubfx x1, x0, #16, #8

; 方式 2：LSR + AND（2 条指令）
lsr  x1, x0, #16            ; 右移 16 位
and  x1, x1, #0xFF          ; 取低 8 位
```

> UBFX 不仅省一条指令，还天然处理了零扩展。SBFX 更是 LSR+AND 无法替代的（需要符号扩展）。

---

## 交叉列表

| 指令 | 提取/插入 | 扩展 | 一条指令 vs 两条 |
|------|----------|------|-----------------|
| `UBFX` | 提取 | 零扩展 | vs `LSR + AND`（2条） |
| `SBFX` | 提取 | 符号扩展 | 无法用 LSR+AND 替代 |
| `BFI` | 插入 | 不变 | vs `AND + ORR`（3条） |
| `BFC` | 清除 | 清零 | vs `AND + 反掩码`（2条） |

---

## HFT 关联

位段操作在协议解析中极其高效：
- 金融协议的位域字段（如 ITCH 消息的 type+flags）用 UBFX 一步提取
- 比 `LSR + AND` 两步操作更快（1 条指令 vs 2 条）
- BFI 用于构造协议头部（如设置标志位 + 类型字段）
- 内核中页表项的属性位提取用 UBFX（如从 PTE 提取 AttrIndx 字段）
- SBFX 用于提取有符号偏移量（如分支指令编码中的跳转偏移）

---

## 自测题

1. 用 UBFX 从 x0 的 bit[31:20] 提取 12 位，写出指令。
<details><summary>答案</summary>

```asm
ubfx x1, x0, #20, #12
```

参数顺序：目标、源、最低位号、宽度。

</details>

2. UBFX 和 SBFX 的区别？提取 `bit[7:0]=0xFF` 时结果有何不同？
<details><summary>答案</summary>

UBFX 零扩展，结果 = 0x000000FF(255)。SBFX 符号扩展，bit[7]=1 → 结果 = 0xFFFFFFFF(-1)。选择哪个取决于字段是有符号还是无符号。

</details>

3. 如何用 BFI 将 x1 的低 4 位插入 x0 的 bit[7:4]？
<details><summary>答案</summary>

```asm
bfi x0, x1, #4, #4
```

x0 的 bit[7:4] 被 x1 的 bit[3:0] 替换，其余位不变。

</details>

4. `SBFX x1, x0, #5, #19` 提取的是有符号还是无符号字段？结果如何扩展？
<details><summary>答案</summary>

**有符号**。SBFX 符号扩展：提取 x0 的 bit[23:5]（19 位），如果最高位（bit 23）= 1，高位全补 1（负数）；否则高位全补 0（正数）。

</details>

5. 为什么说 UBFX 比 `LSR + AND` 更好？
<details><summary>答案</summary>

两个原因：(1) UBFX 一条指令完成，LSR+AND 需要两条；(2) UBFX 天然做零扩展，不需要额外指令。更重要的是 SBFX 做符号扩展，LSR+AND 无法替代。

</details>

6. 如何用 BFI 构造一个 32 位值，包含 4 位 type + 8 位 flags + 20 位 reserved？
<details><summary>答案</summary>

```asm
mov  x0, xzr                ; 清零
bfi  x0, x1, #0, #4          ; type → bit[3:0]
bfi  x0, x2, #4, #8         ; flags → bit[11:4]
; bit[31:12] 保持 0（reserved）
```

</details>

---

## 参考与延伸

- 原书 §4.5
- [4.4 位操作指令](04-bit-ops.md)
- [4.6 典型例子](06-examples.md)
- [Ch14 页表项格式](../../chapter-14-memory-management/notes/section-0-本章完整概述.md)
