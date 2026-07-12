# Ch 5 · 加载、存储与寻址

> ***ARM Assembly Language*** — William Sw Smith · **精读**  
> **English:** Loads, Stores, and Addressing

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
| **§5.1** | 简介 | [notes/section-5-1-intro.md](./notes/section-5-1-intro.md) |
| **§5.2** | 内存 | [notes/section-5-2-memory.md](./notes/section-5-2-memory.md) |
| **§5.3** | 加载与存储指令 | [notes/section-5-3-load-store.md](./notes/section-5-3-load-store.md) |
| **§5.4** | 操作数寻址 — 前变址 · 后变址 | [notes/section-5-4-addressing.md](./notes/section-5-4-addressing.md) |
| **§5.5** | 字节序 (Endianness) | [notes/section-5-5-endianness.md](./notes/section-5-5-endianness.md) |
| **§5.6** | 位带内存 (Bit-Banded Memory) — Cortex-M | [notes/section-5-6-bit-banded.md](./notes/section-5-6-bit-banded.md) |
| **§5.7** | 内存注意事项 | [notes/section-5-7-memory-notes.md](./notes/section-5-7-memory-notes.md) |
| **§5.8** | 练习题 | [notes/section-5-8-exercises.md](./notes/section-5-8-exercises.md) |

---

## 本章 Checklist

- [ ] 读完原书对应章
- [ ] 在 `notes/` 写下可复述的要点
- [ ] （若 **精读**）能对照 [02 C](../../02-c-programming/) 或内核 `.S` 举例

---

← [Ch 4](../chapter-04-assembler-rules-directives/) · 下一章 [Ch 6](../chapter-06-constants-literal-pools/) · [OUTLINE](../OUTLINE.md) · [19 README](../README.md)
