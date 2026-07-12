# Ch 6 · 常量与文字池

> ***ARM Assembly Language*** — William Sw Smith · **选读**  
> **English:** Constants and Literal Pools

---

## 本章定位

<!-- 读完后补充：要点、与 20 U-Boot / 21 驱动的衔接 -->

| | |
|---|---|
| **阅读标签** | **选读**（见 [OUTLINE](../OUTLINE.md)） |
| **架构** | 本书 **v4T / v7-M**；AArch64 主书见 [奔跑吧 ARM64](../arm64-programming-practice/) |

---

## 小节笔记

| 小节 | 标题 | 笔记 |
|------|------|------|
| **§6.1** | 简介 | [notes/section-6-1-intro.md](./notes/section-6-1-intro.md) |
| **§6.2** | ARM 循环移位方案 — 常数编码进指令 | [notes/section-6-2-rotate-constants.md](./notes/section-6-2-rotate-constants.md) |
| **§6.3** | 加载常量 — MOVW/MOVT | [notes/section-6-3-load-constants.md](./notes/section-6-3-load-constants.md) |
| **§6.4** | 文字池 (Literal Pools) | [notes/section-6-4-literal-pools.md](./notes/section-6-4-literal-pools.md) |
| **§6.5** | 向寄存器加载地址 | [notes/section-6-5-load-addresses.md](./notes/section-6-5-load-addresses.md) |
| **§6.6** | 练习题 | [notes/section-6-6-exercises.md](./notes/section-6-6-exercises.md) |

---

## 本章 Checklist

- [ ] 读完原书对应章
- [ ] 在 `notes/` 写下可复述的要点
- [ ] （若 **精读**）能对照 [02 C](../../02-c-programming/) 或内核 `.S` 举例

---

← [Ch 5](../chapter-05-loads-stores-addressing/) · 下一章 [Ch 7](../chapter-07-integer-logic-arithmetic/) · [OUTLINE](../OUTLINE.md) · [19 README](../README.md)
