# 第 3 章 · A64指令集1——加载与存储指令

> **《ARM64体系结构编程与实践》** · 奔跑吧Linux社区 · 人民邮电出版社 · **精读**

---

## 本章定位

**Load-Store**：只有 LDR/STR（及变体）访存；ALU 只碰寄存器。寻址模式、STP/LDP 栈帧、`ldr =` 伪指令为本节高频点。

| | |
|---|---|
| **阅读标签** | **精读**（见 [OUTLINE](../OUTLINE.md)） |
| **实验** | **QEMU** `-cpu cortex-a76`（见 [PI5-ADAPT](../PI5-ADAPT.md)）；不必上 Pi5 裸机 |
| **代码** | [arm64_programming_practice](https://github.com/runninglinuxkernel/arm64_programming_practice) |

---

## 小节笔记

| 笔记 | 说明 |
|------|------|
| [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md) | **Ch3 完整总结 · 加载与存储 LDR / STR** |
| [notes/01-load-store-rules.md](./notes/01-load-store-rules.md) | 3.1 Load-Store 核心规则 |
| [notes/02-register-width.md](./notes/02-register-width.md) | 3.2 寄存器宽度与访存宽度 |
| [notes/03-addressing-modes.md](./notes/03-addressing-modes.md) | 3.3 五大寻址模式 |
| [notes/04-stp-ldp.md](./notes/04-stp-ldp.md) | 3.4 LDP / STP 栈操作主力 |
| [notes/05-ldr-pseudo.md](./notes/05-ldr-pseudo.md) | 3.5 LDR 伪指令 ldr =label |
| [notes/06-special-load-store.md](./notes/06-special-load-store.md) | 3.6 特殊访存指令 |
| [notes/07-typical-patterns.md](./notes/07-typical-patterns.md) | 3.7 典型实操模式 |
| [notes/08-pitfalls.md](./notes/08-pitfalls.md) | 3.8 易错点清单 |

---

## 本章 Checklist

- [x] 读完原书对应章（概念 + 样例）
- [ ] QEMU+GDB 单步验证前/后变基与 STP/LDP
- [ ] 能口头答出 § 思考题五道

---

← [Ch 2](../chapter-02-raspberry-pi-lab/) · 下一章 [Ch 4](../chapter-04-a64-arithmetic-shift/) · [OUTLINE](../OUTLINE.md) · [本书导读](../README.md) · [全书总结](../BOOK-SUMMARY.md)
