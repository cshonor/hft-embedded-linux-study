# 第 12 章 · 中断处理

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
| [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md) | **Ch12 完整总结 · 中断处理** |
| [notes/01-interrupt-flow.md](./notes/01-interrupt-flow.md) | §12.1 中断处理全流程 |
| [notes/02-daif.md](./notes/02-daif.md) | §12.2 中断屏蔽（DAIF） |
| [notes/03-timer-interrupt.md](./notes/03-timer-interrupt.md) | §12.3 通用定时器中断 |
| [notes/04-context-save.md](./notes/04-context-save.md) | §12.4 中断现场保存/恢复 |
| [notes/05-irq-controller.md](./notes/05-irq-controller.md) | §12.5 中断控制器演进 |
| [notes/06-lab.md](./notes/06-lab.md) | §12.6 实验要点 |
| [notes/07-pitfalls.md](./notes/07-pitfalls.md) | §12.7 易错点清单 |

---

## 本章 Checklist

- [x] 读完原书对应章
- [x] 完成书中实验（若有）
- [x] 在 `notes/` 记录可复述要点

---

← [Ch 11](../chapter-11-exception-handling/) · 下一章 [Ch 13](../chapter-13-gic-v2/) · [OUTLINE](../OUTLINE.md) · [本书导读](../README.md) · [19 模块](../../README.md)
