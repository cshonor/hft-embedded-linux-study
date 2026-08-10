# 第 18 章 · 内存屏障指令

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
| [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md) | **Ch18 完整总结 · 内存屏障指令** |
| [notes/01-weak-memory-model.md](./notes/01-weak-memory-model.md) | §18.1 弱序内存模型 |
| [notes/02-three-barriers.md](./notes/02-three-barriers.md) | §18.2 三条屏障指令 |
| [notes/03-typical-scenarios.md](./notes/03-typical-scenarios.md) | §18.3 典型场景 |
| [notes/04-acquire-release.md](./notes/04-acquire-release.md) | §18.4 Acquire / Release 语义 |
| [notes/05-linux-barrier-api.md](./notes/05-linux-barrier-api.md) | §18.5 Linux 内核屏障 API |
| [notes/06-lab.md](./notes/06-lab.md) | §18.6 实验要点 |
| [notes/07-pitfalls.md](./notes/07-pitfalls.md) | §18.7 易错点清单 |

---

## 本章 Checklist

- [x] 读完原书对应章
- [x] 完成书中实验（若有）
- [x] 在 `notes/` 记录可复述要点

---

← [Ch 17](../chapter-17-tlb-management/) · 下一章 [Ch 19](../chapter-19-barrier-usage/) · [OUTLINE](../OUTLINE.md) · [本书导读](../README.md) · [19 模块](../../README.md)
