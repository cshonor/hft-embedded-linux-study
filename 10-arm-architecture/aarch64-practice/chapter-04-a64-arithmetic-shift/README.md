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
| [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md) | **本章详细总结**（算术 · NZCV · 移位 · 位段 · 思考题） |
| [../NZCV.md](../NZCV.md) | **NZCV 专篇**（N/Z/C/V · 条件后缀 · 例题） |
| [../SIGNED-UNSIGNED.md](../SIGNED-UNSIGNED.md) | 有符号/无符号：补码 · C vs V · LSR/ASR |

---

## 本章 Checklist

- [x] 读完原书对应章
- [ ] GDB 单步观察 ADDS/SUBS 后 NZCV
- [ ] 能口头答出思考题五道

---

← [Ch 3](../chapter-03-a64-load-store/) · 下一章 [Ch 5](../chapter-05-a64-compare-branch/) · [OUTLINE](../OUTLINE.md) · [本书导读](../README.md) · [全书总结](../BOOK-SUMMARY.md)
