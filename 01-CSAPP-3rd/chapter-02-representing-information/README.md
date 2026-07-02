# Ch 2 信息的表示和处理 · Representing and Manipulating Information

> **CSAPP 3rd** · Bryant & O'Neill · **选读 🟡**（Part I）

> 本章定位：**比特在内存里到底长什么样** — 整数、浮点、字节序、位运算。全书默认你会这些；HFT 不必通读，但 **解析二进制协议、定价格精度、避免溢出** 时本章是地基。

> **笔记体例：** 每节含 **概念 + HFT 关联 + 口述巩固（自测）** — 口述内容来自学习对话，已整理进正文，可直接当复习提纲。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 2.1.1–2.1.3 十六进制、数据大小、字节序 | [notes/section-2.1.1-2.1.3-十六进制寻址与字节序.md](./notes/section-2.1.1-2.1.3-十六进制寻址与字节序.md) · [Ch2 demo](../code/ch02-endian-and-padding-demo.c) |
| 2.1.4–2.1.9 字符串、代码、布尔与 C 位/逻辑/移位 | [notes/section-2.1.4-2.1.9-字符串布尔与C位操作.md](./notes/section-2.1.4-2.1.9-字符串布尔与C位操作.md) |
| 2.2 整数表示（2.2.1–2.2.8） | [notes/section-2.2-整数表示与类型转换.md](./notes/section-2.2-整数表示与类型转换.md) |
| 2.3 整数算术（2.3.1–2.3.8） | [notes/section-2.3-整数算术.md](./notes/section-2.3-整数算术.md) |
| 2.4.1–2.4.4 二进制小数、IEEE 格式与舍入 | [notes/section-2.4.1-2.4.4-IEEE浮点格式与舍入.md](./notes/section-2.4.1-2.4.4-IEEE浮点格式与舍入.md) |
| 2.4.5–2.4.6 浮点运算与 C 浮点 | [notes/section-2.4.5-2.4.6-浮点运算与C浮点.md](./notes/section-2.4.5-2.4.6-浮点运算与C浮点.md) |

---

## 大白话 · 本章一条线

> **程序里的 `int`、`double`、字符串，在内存里都是一串 0/1；算术规则是「按位 + 固定宽度」算出来的，不是数学上的实数。**

```
同一段 32 bit 比特
  → 读成 uint32_t：0 … 2³²−1
  → 读成 int32_t（补码）：−2³¹ … 2³¹−1
  → 读成 float：IEEE 754 语义（完全另一套）
```

**HFT 三个高频场景：**

1. **行情/订单二进制协议** — 大端/小端、字段对齐、`int64` 价格 tick（→ 2.1.3、2.1 位操作）
2. **数量/索引溢出** — `uint32` 累加、有符号比较陷阱（→ 2.2–2.3）
3. **价格用 float 还是定点** — 舍入与非结合性（→ 2.4；生产常 **整数 tick / fixed-point**）

---

## 本章 Checklist

- [ ] 口述自测：§2.1.3 五题、`sizeof(long)` 在 LP64 vs LLP64
- [ ] 解释 **小端 / 大端**；给定 4 字节写出 `0x12345678` 在内存中的布局
- [ ] 写出补码下 −1、INT_MIN 的位模式；无符号与有符号比较坑（` -1 < 0U `）
- [ ] 说明 **符号扩展 vs 零扩展**；截断高位会发生什么
- [ ] 无符号加法溢出 = 模 2ⁿ；补码加法溢出判据（同号相加变号）
- [ ] 读 IEEE 754 单精度：符号 / 阶码 / 尾数；何为 NaN、Inf
- [ ] 能解释为何 `0.1 + 0.2 != 0.3`；HFT 中何时禁用 float 累加 PnL

---

## HFT 精读捷径

```
必读切片：2.1.3 字节序 + 2.1.7 位操作 + 2.2.3 补码 + 2.3 溢出直觉
协议/解码为主 → 加读 2.1.4 字符串、2.2.6–2.2.7 扩展截断
策略里用 double → 精读 2.4；否则 2.4 扫 IEEE 与舍入即可
```

**时间紧：** 2.2–2.3 核心 + 2.1.3；2.4 浮点按需；2.1.6 布尔代数可跳过（与数字电路重叠）。

---

## 相关章节

- 上一章：[../chapter-01-tour-of-computer-systems/](../chapter-01-tour-of-computer-systems/)
- 下一章：[chapter-03-machine-level-programs/](./chapter-03-machine-level-programs/)
- 机器级运算：[../chapter-03-machine-level-programs/](../chapter-03-machine-level-programs/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
