# 4.5 位段提取与插入

> 来源：§4.5 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

UBFX/SBFX/BFI 等位段操作指令，在内核和协议解析中的高频应用。

## 核心要点

| 指令 | 作用 |
|------|------|
| UBFX | 无符号位段提取（零扩展） |
| SBFX | 有符号位段提取（符号扩展） |
| BFI | 位段插入 |
| BFC | 位段清除 |

```asm
; 从 x0 的 bit[23:16] 提取 8 位
ubfx x1, x0, #16, #8

; 将 x1 的低 8 位插入 x0 的 bit[23:16]
bfi x0, x1, #16, #8
```

## HFT 关联

位段操作在协议解析中极其高效：
- 金融协议的位域字段（如 ITCH 消息的 type+flags）用 UBFX 一步提取
- 比 `LSR + AND` 两步操作更快（1 条指令 vs 2 条）
- BFI 用于构造协议头部（如设置标志位 + 类型字段）
- 内核中页表项的属性位提取也用 UBFX（如从 PTE 提取 AttrIndx 字段）

## 自测题

1. 用 UBFX 从 x0 的 bit[31:20] 提取 12 位，写出指令。
<details><summary>答案</summary>
```asm
ubfx x1, x0, #20, #12
```
参数顺序：目标、源、最低位号、宽度。
</details>

2. UBFX 和 SBFX 的区别？提取 bit[7:0]=0xFF 时结果有何不同？
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

## 参考与延伸

- 原书 §4.5
- [4.4 位操作](04-bit-ops.md)
- [Ch14 页表项格式](../../chapter-14-memory-management/notes/section-0-本章完整概述.md)
