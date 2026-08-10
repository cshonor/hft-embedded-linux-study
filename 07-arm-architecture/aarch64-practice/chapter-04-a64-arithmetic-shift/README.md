# 第 4 章 · A64指令集2——算术与移位指令

> **《ARM64体系结构编程与实践》** · 奔跑吧Linux社区 · 人民邮电出版社 · **精读**

---

## 本章定位

ADD/ADDS、SUB/SUBS/CMP、NZCV、移位、AND/ORR/EOR/BIC、UBFX/SBFX/BFI；仍遵守 Load-Store（只碰寄存器）。

| | |
|---|---|
| **阅读标签** | **精读**（见 [OUTLINE](../OUTLINE.md)） |
| **实验** | **QEMU** `-cpu cortex-a76`（见 [PI5-ADAPT](../PI5-ADAPT.md)） |
| **代码** | [arm64_programming_practice](https://github.com/runninglinuxkernel/arm64_programming_practice) |

---

## 小节笔记

| 笔记 | 说明 |
|------|------|
| [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md) | **Ch4 完整总结 · 算术、移位与位操作** |
| [notes/01-arithmetic.md](./notes/01-arithmetic.md) | 4.1 算术指令 |
| [notes/02-nzcv.md](./notes/02-nzcv.md) | 4.2 NZCV 四个条件标志 |
| [notes/03-shift.md](./notes/03-shift.md) | 4.3 移位指令 |
| [notes/04-bit-ops.md](./notes/04-bit-ops.md) | 4.4 位操作指令 |
| [notes/05-bit-field.md](./notes/05-bit-field.md) | 4.5 位段提取与插入 |
| [notes/06-examples.md](./notes/06-examples.md) | 4.6 典型例子 |
| [notes/07-pitfalls.md](./notes/07-pitfalls.md) | 4.7 易错坑 |

---

## 本章 Checklist

- [x] 读完原书对应章
- [ ] GDB 单步观察 ADDS/SUBS 后 NZCV
- [ ] 能口头答出思考题五道

---

← [Ch 3](../chapter-03-a64-load-store/) · 下一章 [Ch 5](../chapter-05-a64-compare-branch/) · [OUTLINE](../OUTLINE.md) · [本书导读](../README.md) · [全书总结](../BOOK-SUMMARY.md)
