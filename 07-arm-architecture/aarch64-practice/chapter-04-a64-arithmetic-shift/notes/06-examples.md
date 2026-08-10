# 4.6 典型例子

> 来源：§4.6 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

算术/移位/位操作的综合应用案例。

## 核心要点

### 对齐计算
```asm
; 将 x0 向上对齐到 16 字节边界
add x0, x0, #15
bic x0, x0, #0xF      ; 清除低 4 位 = 向下取整到 16
```

### 位域打包
```asm
; 将 type(x1, 4bit) 和 flags(x2, 8bit) 打包到 x0
mov x0, xzr
bfi x0, x1, #0, #4     ; type → bit[3:0]
bfi x0, x2, #4, #8     ; flags → bit[11:4]
```

### 绝对值
```asm
; x0 = |x1|
subs x2, x1, #0        ; 设置标志
cneg x0, x1, mi        ; 如果 N=1(负数)，取反
```

## HFT 关联

这些模式在 HFT 代码中高频出现：
- 对齐计算：DMA buffer 地址对齐 → `bic x0, x0, #0xF`
- 位域打包：构造订单消息头 → BFI 一步插入字段
- 绝对值：计算价格变动 abs(new - old) → CNEG 条件取反无分支
- 无分支代码消除分支预测失败 → 减少流水线气泡

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

## 参考与延伸

- 原书 §4.6
- [4.5 位段提取](05-bit-field.md)
- [5.2 条件选择指令](../../chapter-05-a64-compare-branch/notes/section-0-本章完整概述.md)
