# Ch 13 · 子程序与堆栈

> ***ARM Assembly Language*** — William Sw Smith · **精读**  
> **English:** Subroutines and Stacks

---

## 本章定位

<!-- 读完后补充：要点、与 20 U-Boot / 21 驱动的衔接 -->

| | |
|---|---|
| **阅读标签** | **精读**（见 [OUTLINE](../OUTLINE.md)） |
| **架构** | 本书 **v4T / v7-M**；AArch64 主书见 [奔跑吧 ARM64](../arm64-programming-practice/) |

---

## 小节笔记

| 小节 | 标题 | 笔记 |
|------|------|------|
| **§13.1** | 简介 | [notes/section-13-1-intro.md](./notes/section-13-1-intro.md) |
| **§13.2** | 堆栈 — LDM/STM · PUSH/POP · 满/空 · 递增/递减 | [notes/section-13-2-stacks.md](./notes/section-13-2-stacks.md) |
| **§13.3** | 子程序 | [notes/section-13-3-subroutines.md](./notes/section-13-3-subroutines.md) |
| **§13.4** | 向子程序传递参数 — 寄存器 · 指针 · 堆栈 | [notes/section-13-4-parameters.md](./notes/section-13-4-parameters.md) |
| **§13.5** | ARM APCS — 应用过程调用标准 | [notes/section-13-5-apcs.md](./notes/section-13-5-apcs.md) |
| **§13.6** | 练习题 | [notes/section-13-6-exercises.md](./notes/section-13-6-exercises.md) |

---

## 本章 Checklist

- [ ] 读完原书对应章
- [ ] 在 `notes/` 写下可复述的要点
- [ ] （若 **精读**）能对照 [02 C](../../02-c-programming/) 或内核 `.S` 举例

---

← [Ch 12](../chapter-12-tables/) · 下一章 [Ch 14](../chapter-14-exception-handling-arm7tdmi/) · [OUTLINE](../OUTLINE.md) · [19 README](../README.md)
