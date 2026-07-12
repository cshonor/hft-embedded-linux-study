# Ch 17 · ARM、Thumb 和 Thumb-2 指令

> ***ARM Assembly Language*** — William Sw Smith · **选读**  
> **English:** ARM, Thumb and Thumb-2 Instructions

---

## 本章定位

| | |
|---|---|
| **角色** | **选读** — **ISA 演进**（ARM7 双态 vs M4 Thumb-2 only） |
| **核心模式** | 密度 **65–70%** · **PLA 解压** · **Thumb-2 16/32** · **`BX` bit0** · **Veneer** |
| **M4 重点** | **§17.3 UAL**；§17.4–17.5 作历史/链接背景 |
| **前置** | [Ch3](../chapter-03-instruction-sets-v4t-v7m/) · [Ch8 BX/IT](../chapter-08-branches-loops/) |

📋 **口述总览** → [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md)

---

## 小节笔记

| 小节 | 标题 | 笔记 |
|------|------|------|
| **§17.1** | 简介 | [notes/section-17-1-intro.md](./notes/section-17-1-intro.md) |
| **§17.2** | ARM 与 16 位 Thumb 指令 | [notes/section-17-2-arm-vs-thumb16.md](./notes/section-17-2-arm-vs-thumb16.md) |
| **§17.3** | 32 位 Thumb 指令 (Thumb-2) | [notes/section-17-3-thumb2.md](./notes/section-17-3-thumb2.md) |
| **§17.4** | ARM 与 Thumb 状态切换 — BX 等 | [notes/section-17-4-state-switch.md](./notes/section-17-4-state-switch.md) |
| **§17.5** | 如何为 Thumb 编译代码 — Interworking | [notes/section-17-5-interworking.md](./notes/section-17-5-interworking.md) |
| **§17.6** | 练习题 | [notes/section-17-6-exercises.md](./notes/section-17-6-exercises.md) |

---

## 本章 Checklist

- [ ] 对比 **ARM 32 / Thumb 16 / Thumb-2** 宽度、条件执行、标志 **S**
- [ ] 说清 **16-bit Thumb 限制**（r0–r7、同源同宿、小立即数）
- [ ] 解释 **Thumb-2 高 5 bit** 如何区分 16 vs 32 bit 指令
- [ ] **`BX`：bit0=0 ARM，bit0=1 Thumb**；**`BL` 不切状态**
- [ ] 说明 **Veneer / Interworking**；**M4 为何不需要**
- [ ] 对照 [Ch15](../chapter-15-exception-handling-v7m/)：**永远 Thumb-2**

---

← [Ch 16](../chapter-16-memory-mapped-peripherals/) · 下一章 [Ch 18](../chapter-18-mixing-c-and-assembly/) · [OUTLINE](../OUTLINE.md) · [19 README](../README.md)
