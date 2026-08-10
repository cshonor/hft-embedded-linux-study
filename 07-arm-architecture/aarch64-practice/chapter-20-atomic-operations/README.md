# 第 20 章 · 原子操作

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
| [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md) | **Ch20 完整总结 · 原子操作** |
| [notes/01-exclusive-monitor.md](./notes/01-exclusive-monitor.md) | §20.1 独占监视器（Exclusive Monitor） |
| [notes/02-atomic-patterns.md](./notes/02-atomic-patterns.md) | §20.2 原子操作实现模式 |
| [notes/03-lse.md](./notes/03-lse.md) | §20.3 ARMv8.1 LSE（Large System Extensions） |
| [notes/04-wfe-sev.md](./notes/04-wfe-sev.md) | §20.4 WFE / SEV —— 低功耗自旋锁 |
| [notes/05-linux-atomic-api.md](./notes/05-linux-atomic-api.md) | §20.5 Linux 原子操作 API |
| [notes/06-lab.md](./notes/06-lab.md) | §20.6 实验要点 |
| [notes/07-pitfalls.md](./notes/07-pitfalls.md) | §20.7 易错点清单 |

---

## 本章 Checklist

- [x] 读完原书对应章
- [x] 完成书中实验（若有）
- [x] 在 `notes/` 记录可复述要点

---

← [Ch 19](../chapter-19-barrier-usage/) · 下一章 [Ch 21](../chapter-21-os-topics/) · [OUTLINE](../OUTLINE.md) · [本书导读](../README.md) · [19 模块](../../README.md)
