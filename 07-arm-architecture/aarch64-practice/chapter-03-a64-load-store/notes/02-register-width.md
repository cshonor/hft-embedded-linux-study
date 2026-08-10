# 3.2 寄存器宽度与访存宽度

> 来源：§3.2 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

LDR/STR 的宽度变体（B/H/W/X）和符号扩展/零扩展的区别。

## 核心要点

| 指令 | 作用 | 宽度 |
|------|------|------|
| `LDR x0, [x1]` | 读 8 字节 | 64 |
| `LDR w0, [x1]` | 读 4 字节，高 32 清零 | 32 |
| `LDRB` | 读 1 字节 | 8 |
| `LDRH` | 读半字 | 16 |

- **S 后缀**：`LDRSB`/`LDRSH` = 符号扩展；不带 S = 零扩展
- B=Byte(8)、H=Half(16)、W=32、X=64
- 写 Wn → 高 32 位自动清零（同 1.3 节寄存器规则）

## HFT 关联

正确选择访存宽度影响性能和正确性：
- 读取市场数据的小端字段（如价格类型 1 字节）用 `LDRB` 比 `LDR`+移位更快
- 符号扩展用 `LDRSB`/`LDRSH` 一步完成，避免额外 `SXTB`/`SXTH` 指令
- 误用 `LDRB` 读取有符号负数会导致高位为 0 而非 1 → 数值错误

## 自测题

1. `LDRSB w0, [x1]` 和 `LDRB w0, [x1]` 读同一个字节 0xFF，结果分别是什么？
<details><summary>答案</summary>
LDRSB（符号扩展）：w0 = 0xFFFFFFFF（-1）。LDRB（零扩展）：w0 = 0x000000FF（255）。
</details>

2. `LDRH x0, [x1]` 执行后，x0 的高 48 位是什么？
<details><summary>答案</summary>
全 0。LDRH 读取 2 字节并零扩展到 64 位，高位自动清零。
</details>

3. 为什么 `LDRSB` 的 S 不要和 `ADDS` 的 S 混淆？
<details><summary>答案</summary>
LDRSB 的 S = Sign-extend（符号扩展），控制读入数据如何扩展到目标宽度。ADDS 的 S = Set flags（设置条件标志 NZCV）。两者完全不同。
</details>

## 参考与延伸

- 原书 §3.2
- [3.1 Load-Store 规则](01-load-store-rules.md)
- [SIGNED-UNSIGNED.md](../../SIGNED-UNSIGNED.md)
