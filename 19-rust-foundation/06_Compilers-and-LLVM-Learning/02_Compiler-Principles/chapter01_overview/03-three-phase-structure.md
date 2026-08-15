# 第 1 章 · 编译总览 · §3 三阶段结构

← [本章目录](./README.md) · 上一节：[02-two-fundamental-principles.md](./02-two-fundamental-principles.md) · 下一节：[04-translation-pipeline-example.md](./04-translation-pipeline-example.md)

---

为应对大型软件复杂性，现代编译器在逻辑上通常采用 **三阶段结构（three-phase compiler）**：

```text
Source Code
    → Front end（前端）
    → Optimizer（优化器）
    → Back end（后端）
    → Target（机器码 / 汇编 / 对象文件 …）
```

---

## 前端（Front end）

| 项目 | 内容 |
|------|------|
| **目标** | **理解**源语言程序 |
| **手段** | 扫描 · 语法分析 · **上下文相关分析**（类型、作用域等） |
| **产出** | 确认程序合法 → 映射为 **IR（Intermediate Representation）** |

→ 与 CI [ch2 §2.1「上山前端」](../../01_Crafting-Interpreters/part01_welcome/chapter02_map-of-the-territory/04-rust-hft-编译流水线对照.md) 一致

---

## 优化器（Optimizer）

| 项目 | 内容 |
|------|------|
| **位置** | 前端与后端之间的**中间层** |
| **输入** | 前端生成的 IR |
| **工作** | 分析 IR 并**改进**（更快、更小、更省电……） |
| **输出** | **改进后的 IR** |

**Rust**：LLVM 优化 Pass 链即工业级优化器层 → [04 Learn LLVM 17](../../../04_Learn-LLVM-17/README.md)

---

## 后端（Back end）

| 项目 | 内容 |
|------|------|
| **目标** | 将 IR **映射到目标机器** |
| **难点** | 目标 **指令集**、**有限寄存器**、**流水线 / 延迟** |
| **产出** | 高效 **机器码**（或汇编、对象文件） |

→ 与 CI [ch2「下山后端」](../../01_Crafting-Interpreters/part01_welcome/chapter02_map-of-the-territory/04-rust-hft-编译流水线对照.md) 一致

---

## 与「单阶段 / 多 Pass」的关系

三阶段是**逻辑划分**；物理实现上可以是多次遍历同一 IR（Pass 管线），但职责仍落在这三块。

| 体系 | 三阶段实例 |
|------|------------|
| **rustc** | 前端（Rust AST/HIR/MIR…）→ LLVM Opt → LLVM 后端 |
| **clox** | 前端编译到字节码 → （简化优化 ch30）→ VM「执行后端」 |
| **jlox** | 仅前端 + **解释执行**（无独立优化器/机器码后端） |
