# 第 7 章 · A64指令集的陷阱

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
| [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md) | **Ch7 完整总结 · A64 指令集的陷阱** |
| [notes/01-mov-trap.md](./notes/01-mov-trap.md) | 7.1 大立即数 MOV 陷阱 |
| [notes/02-string-load.md](./notes/02-string-load.md) | 7.2 字符串加载陷阱 |
| [notes/03-ldxr-crash.md](./notes/03-ldxr-crash.md) | 7.3 LDXR 导致死机 |
| [notes/04-stack-alignment.md](./notes/04-stack-alignment.md) | 7.4 栈对齐陷阱 |
| [notes/05-condition-trap.md](./notes/05-condition-trap.md) | 7.5 条件执行陷阱 |
| [notes/06-linux-boot-asm.md](./notes/06-linux-boot-asm.md) | 7.6 Linux 启动汇编分析（大作业） |
| [notes/07-uart-output.md](./notes/07-uart-output.md) | 7.7 串口输出实验 |
| [notes/08-pitfalls.md](./notes/08-pitfalls.md) | 7.8 易错点清单 |

---

## 本章 Checklist

- [x] 读完原书对应章
- [x] 完成书中实验（若有）
- [x] 在 `notes/` 记录可复述要点

---

← [Ch 6](../chapter-06-a64-other-instructions/) · 下一章 [Ch 8](../chapter-08-gnu-assembler/) · [OUTLINE](../OUTLINE.md) · [本书导读](../README.md) · [19 模块](../../README.md)
