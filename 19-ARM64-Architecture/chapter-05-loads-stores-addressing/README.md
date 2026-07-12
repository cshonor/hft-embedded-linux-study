# Ch 5 · 加载、存储与寻址

> ***ARM Assembly Language*** — William Sw Smith · **精读**  
> **English:** Loads, Stores, and Addressing

---

## 本章定位

| | |
|---|---|
| **角色** | **精读** — RISC 铁律 **Load→算→Store** · 寻址 · Endian · MMIO 基础 |
| **量级** | 动态指令约 **一半** 是 Load/Store |
| **选读** | §5.6 位带 — M3/M4 专用；Linux/A 路径理解 R-M-W 即可 |

📋 **口述总览** → [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md)

**前置：** [Ch4 伪指令](../chapter-04-assembler-rules-directives/notes/section-0-本章完整概述.md) · [Ch2 对齐](../chapter-02-programmers-model/notes/section-2-2-data-types.md)

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

- [ ] 熟练使用 **LDR/STR · LDRH/STRH · LDRB/STRB · LDRSH/LDRSB**
- [ ] 写对 **Pre `[Rn,#off]!`** 与 **Post `[Rn], #off`**
- [ ] 用 **`[Rn, Rm, LSL #2]`** 访问 `arr[i]`
- [ ] 解释 **little-endian** 与 **`REV`**
- [ ] 说清 **链接脚本 / Scatter** 如何把代码与变量放进 Flash/RAM

---

← [Ch 4](../chapter-04-assembler-rules-directives/) · 下一章 [Ch 6](../chapter-06-constants-literal-pools/) · [OUTLINE](../OUTLINE.md) · [19 README](../README.md)
