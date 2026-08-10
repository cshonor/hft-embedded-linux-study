# 第 11 章 · 异常处理

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
| [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md) | **Ch11 完整总结 · 异常处理** |
| [notes/01-exception-types.md](./notes/01-exception-types.md) | §11.1 异常类型 |
| [notes/02-el-switch.md](./notes/02-el-switch.md) | §11.2 异常等级切换 |
| [notes/03-vector-table.md](./notes/03-vector-table.md) | §11.3 异常向量表（VBAR） |
| [notes/04-hw-sw-save.md](./notes/04-hw-sw-save.md) | §11.4 硬件保存 + 软件保存 |
| [notes/05-esr.md](./notes/05-esr.md) | §11.5 异常综合征（ESR） |
| [notes/06-el2-to-el1.md](./notes/06-el2-to-el1.md) | §11.6 EL2 → EL1 实验 |
| [notes/07-lab.md](./notes/07-lab.md) | §11.7 实验要点 |
| [notes/08-pitfalls.md](./notes/08-pitfalls.md) | §11.8 易错点清单 |

---

## 本章 Checklist

- [x] 读完原书对应章
- [x] 完成书中实验（若有）
- [x] 在 `notes/` 记录可复述要点

---

← [Ch 10](../chapter-10-gcc-inline-asm/) · 下一章 [Ch 12](../chapter-12-interrupt-handling/) · [OUTLINE](../OUTLINE.md) · [本书导读](../README.md) · [19 模块](../../README.md)
