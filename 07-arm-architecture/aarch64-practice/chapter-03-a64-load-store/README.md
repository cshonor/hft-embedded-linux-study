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
| [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md) | **本章详细总结**（寻址 · LDP/STP · 伪 LDR · 思考题） |
| [../SIGNED-UNSIGNED.md](../SIGNED-UNSIGNED.md) | 有符号/无符号：LDRB vs LDRSB、补码 |
| [../S-SUFFIX.md](../S-SUFFIX.md) | LDRSB 的 S vs ADDS 的 S（勿混） |

---

## 本章 Checklist

- [x] 读完原书对应章（概念 + 样例）
- [ ] QEMU+GDB 单步验证前/后变基与 STP/LDP
- [ ] 能口头答出 § 思考题五道

---

← [Ch 2](../chapter-02-raspberry-pi-lab/) · 下一章 [Ch 4](../chapter-04-a64-arithmetic-shift/) · [OUTLINE](../OUTLINE.md) · [本书导读](../README.md) · [全书总结](../BOOK-SUMMARY.md)
