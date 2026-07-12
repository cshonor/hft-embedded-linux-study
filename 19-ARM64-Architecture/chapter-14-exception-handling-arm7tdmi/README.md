# Ch 14 · 异常处理：ARM7TDMI

> ***ARM Assembly Language*** — William Sw Smith · **选读**  
> **English:** Exception Handling: ARM7TDMI

---

## 本章定位

<!-- 读完后补充：要点、与 20 U-Boot / 21 驱动的衔接 -->

| | |
|---|---|
| **阅读标签** | **选读**（见 [OUTLINE](../OUTLINE.md)） |
| **架构** | 本书 **v4T / v7-M**；AArch64 主书见 [奔跑吧 ARM64](../arm64-programming-practice/) |

---

## 小节笔记

| 小节 | 标题 | 笔记 |
|------|------|------|
| **§14.1** | 简介 | [notes/section-14-1-intro.md](./notes/section-14-1-intro.md) |
| **§14.2** | 中断 | [notes/section-14-2-interrupts.md](./notes/section-14-2-interrupts.md) |
| **§14.3** | 错误条件 | [notes/section-14-3-error-conditions.md](./notes/section-14-3-error-conditions.md) |
| **§14.4** | 异常序列 | [notes/section-14-4-exception-sequence.md](./notes/section-14-4-exception-sequence.md) |
| **§14.5** | 向量表 | [notes/section-14-5-vector-table.md](./notes/section-14-5-vector-table.md) |
| **§14.6** | 处理程序与优先级 | [notes/section-14-6-handlers-priority.md](./notes/section-14-6-handlers-priority.md) |
| **§14.7** | 基础机制小结 | [notes/section-14-7-mechanism.md](./notes/section-14-7-mechanism.md) |
| **§14.8** | 处理异常的程序 — 复位 · 未定义 · VIC · 中止 · SVC | [notes/section-14-8-handler-code.md](./notes/section-14-8-handler-code.md) |
| **§14.9** | 练习题 | [notes/section-14-9-exercises.md](./notes/section-14-9-exercises.md) |

---

## 本章 Checklist

- [ ] 读完原书对应章
- [ ] 在 `notes/` 写下可复述的要点
- [ ] （若 **精读**）能对照 [02 C](../../02-c-programming/) 或内核 `.S` 举例

---

← [Ch 13](../chapter-13-subroutines-stacks/) · 下一章 [Ch 15](../chapter-15-exception-handling-v7m/) · [OUTLINE](../OUTLINE.md) · [19 README](../README.md)
