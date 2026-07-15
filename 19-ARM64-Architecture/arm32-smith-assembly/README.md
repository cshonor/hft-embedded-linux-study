# *ARM Assembly Language* — Smith（ARM32 汇编思维）

> **书目：** William Sw Smith · *ARM Assembly Language*  
> **架构：** **ARMv4T（ARM7TDMI）** + **ARMv7-M（Cortex-M）** — **不是** AArch64  
> **模块：** [19-ARM64-Architecture](../README.md) · 与 [奔跑吧 ARM64](../arm64-programming-practice/) **并列**

📋 **阅读裁剪与标签** → [OUTLINE.md](./OUTLINE.md)

---

## 本目录放什么

Smith 全书笔记、附录、术语、代码与脚手架 — **ARM32 / Thumb / Cortex-M** 一条线收齐：

| 内容 | 路径 |
|------|------|
| 章节 Ch1–18 | `chapter-01-…` … `chapter-18-…` |
| 附录 A–D | `appendix-*` |
| 术语 / 参考 / 示例代码 | `glossary/` · `references/` · `code/` |
| 生成脚本 | `_scripts/` |

**AArch64 不在这里** → [../arm64-programming-practice/](../arm64-programming-practice/)

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

学完本目录再进 [奔跑吧 OUTLINE](../arm64-programming-practice/OUTLINE.md)，概念可一一映射。

← [19 模块 README](../README.md)
