# 第 22 章 · 浮点运算与NEON指令

> **《ARM64体系结构编程与实践》** · 奔跑吧Linux社区 · 人民邮电出版社 · **选读**

---

## 本章定位

本章详细总结见 [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md)。

| | |
|---|---|
| **阅读标签** | **选读**（见 [OUTLINE](../OUTLINE.md)） |
| **实验** | 树莓派 4B / **QEMU ARM64**（官方仓库 [arm64_programming_practice](https://github.com/runninglinuxkernel/arm64_programming_practice)） |

---

## 小节笔记

| 笔记 | 说明 |
|------|------|
| [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md) | **Ch22 完整总结 · 浮点运算与 NEON 指令** |
| [notes/01-fp-registers.md](./notes/01-fp-registers.md) | §22.1 浮点寄存器 |
| [notes/02-neon-vectors.md](./notes/02-neon-vectors.md) | §22.2 NEON 向量寄存器 |
| [notes/03-neon-instructions.md](./notes/03-neon-instructions.md) | §22.3 常用 NEON 指令 |
| [notes/04-rgb-bgr.md](./notes/04-rgb-bgr.md) | §22.4 RGB → BGR 转换示例 |
| [notes/05-matrix-multiply.md](./notes/05-matrix-multiply.md) | §22.5 矩阵乘法加速 |
| [notes/06-intrinsics.md](./notes/06-intrinsics.md) | §22.6 NEON 内建函数（Intrinsics） |
| [notes/07-lab.md](./notes/07-lab.md) | §22.7 实验要点 |
| [notes/08-pitfalls.md](./notes/08-pitfalls.md) | §22.8 易错点清单 |

---

## 本章 Checklist

- [x] 读完原书对应章
- [x] 完成书中实验（若有）
- [x] 在 `notes/` 记录可复述要点

---

← [Ch 21](../chapter-21-os-topics/) · 下一章 [Ch 23](../chapter-23-sve-optimization/) · [OUTLINE](../OUTLINE.md) · [本书导读](../README.md) · [19 模块](../../README.md)
