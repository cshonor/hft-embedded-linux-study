# 第 5 章 · A64指令集3——比较指令与跳转指令

> **《ARM64体系结构编程与实践》** · 奔跑吧Linux社区 · 人民邮电出版社 · **精读**

---

## 本章定位

本章详细总结见 [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md)。

| | |
|---|---|
| **阅读标签** | **精读**（见 [OUTLINE](../OUTLINE.md)） |
| **实验** | 树莓派 4B / **QEMU ARM64**（官方仓库 [arm64_programming_practice](https://github.com/runninglinuxkernel/arm64_programming_practice)） |

---

## 小节笔记

| 笔记 | 说明 |
|------|------|
| [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md) | **Ch5 完整总结 · 比较指令与跳转指令** |
| [notes/01-compare.md](./notes/01-compare.md) | 5.1 比较指令 |
| [notes/02-csel.md](./notes/02-csel.md) | 5.2 条件选择指令 CSEL/CSET |
| [notes/03-branch.md](./notes/03-branch.md) | 5.3 跳转指令全览 |
| [notes/04-condition-suffix.md](./notes/04-condition-suffix.md) | 5.4 条件后缀速查 |
| [notes/05-cbz-tbz.md](./notes/05-cbz-tbz.md) | 5.5 CBZ / CBNZ / TBZ / TBNZ |
| [notes/06-patterns.md](./notes/06-patterns.md) | 5.6 典型代码模式 |
| [notes/07-lab.md](./notes/07-lab.md) | 5.7 实验要点 |
| [notes/08-pitfalls.md](./notes/08-pitfalls.md) | 5.8 易错点清单 |

---

## 本章 Checklist

- [x] 读完原书对应章
- [x] 完成书中实验（若有）
- [x] 在 `notes/` 记录可复述要点

---

← [Ch 4](../chapter-04-a64-arithmetic-shift/) · 下一章 [Ch 6](../chapter-06-a64-other-instructions/) · [OUTLINE](../OUTLINE.md) · [本书导读](../README.md) · [19 模块](../../README.md)
