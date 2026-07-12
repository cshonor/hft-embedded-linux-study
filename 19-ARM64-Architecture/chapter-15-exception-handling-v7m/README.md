# Ch 15 · 异常处理：v7-M

> ***ARM Assembly Language*** — William Sw Smith · **选读**  
> **English:** Exception Handling: v7-M

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
| **§15.1** | 简介 | [notes/section-15-1-intro.md](./notes/section-15-1-intro.md) |
| **§15.2** | 操作模式与特权级别 | [notes/section-15-2-modes-privilege.md](./notes/section-15-2-modes-privilege.md) |
| **§15.3** | 向量表 | [notes/section-15-3-vector-table.md](./notes/section-15-3-vector-table.md) |
| **§15.4** | 堆栈指针 — MSP/PSP | [notes/section-15-4-stack-pointers.md](./notes/section-15-4-stack-pointers.md) |
| **§15.5** | 处理器出入栈序列 | [notes/section-15-5-stack-frames.md](./notes/section-15-5-stack-frames.md) |
| **§15.6** | 异常类型 — 硬故障 · 内存管理故障等 | [notes/section-15-6-fault-types.md](./notes/section-15-6-fault-types.md) |
| **§15.7** | 中断 — 基于 NVIC 的外部中断 | [notes/section-15-7-nvic.md](./notes/section-15-7-nvic.md) |
| **§15.8** | 练习题 | [notes/section-15-8-exercises.md](./notes/section-15-8-exercises.md) |

---

## 本章 Checklist

- [ ] 读完原书对应章
- [ ] 在 `notes/` 写下可复述的要点
- [ ] （若 **精读**）能对照 [02 C](../../02-c-programming/) 或内核 `.S` 举例

---

← [Ch 14](../chapter-14-exception-handling-arm7tdmi/) · 下一章 [Ch 16](../chapter-16-memory-mapped-peripherals/) · [OUTLINE](../OUTLINE.md) · [19 README](../README.md)
