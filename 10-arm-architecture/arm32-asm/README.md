# *ARM Assembly Language: Fundamentals and Techniques*（ARM32 汇编思维）

> **书目：** William **Hohl**、Christopher **Hinds** · *ARM Assembly Language Fundamentals and Techniques*, 2nd ed  
> （目录旧称 “Smith” 已更正；章节骨架仍对应本书。）  
> **全书总结：** [BOOK-SUMMARY.md](./BOOK-SUMMARY.md) · **A 核边界：** [CORTEX-A-SCOPE.md](./CORTEX-A-SCOPE.md)  
> **架构：** 实操 **ARMv4T + v7-M（Cortex-M）**；Cortex-A 仅对比科普 — **不是** AArch64 / 树莓派课  
> **模块：** [10-arm-architecture](../README.md) · 与 [奔跑吧 ARM64](../aarch64-practice/) **并列**

📋 **阅读裁剪与标签** → [OUTLINE.md](./OUTLINE.md)

---

## 本目录放什么

Hohl/Hinds 全书笔记、附录、术语、代码与脚手架 — **ARM32 / Thumb / Cortex-M** 一条线收齐（**不是** Cortex-A Linux 板课）：

| 内容 | 路径 |
|------|------|
| 章节 Ch1–18 | `chapter-01-…` … `chapter-18-…` |
| 附录 A–D | `appendix-*` |
| 术语 / 参考 / 示例代码 | `glossary/` · `references/` · `code/` |
| 生成脚本 | `_scripts/` |

**AArch64 不在这里** → [../aarch64-practice/](../aarch64-practice/)

---

## 推荐精读（嵌入式支线）

| 章 | 主题 | 标签 |
|----|------|------|
| **2** | 程序员模型（7 模式 · M4 Thread/Handler） | **精读** |
| **3–5** | 指令入门 · 汇编规则 · Load/Store | **精读** |
| **7–8** | 整数运算 · 标志 · 分支循环 | **精读** |
| **13** | 子程序与栈 | **精读** |
| **16** | MMIO 外设 | **精读** |
| **18** | C 与汇编混合 | **精读** |
| 9–11 | 浮点 | 跳过（多数） |
| 14–15 · 17 | 异常 · Thumb 细节 | 选读 |

全文表 → [OUTLINE.md](./OUTLINE.md)

---

## 与 ARM64 的边界

| | 本目录（Smith） | 奔跑吧 |
|--|-----------------|--------|
| ISA | ARM32 / Thumb-2 | **A64** |
| 特权模型 | 7 模式 / Thread·Handler | **EL0–EL3** |
| 中断 | IRQ/FIQ · NVIC | **GIC** |

学完本目录再进 [奔跑吧 OUTLINE](../aarch64-practice/OUTLINE.md)，概念可一一映射。

← [19 模块 README](../README.md) · [全书总结](./BOOK-SUMMARY.md)
