# 第 5 章 · 中间表示 · §1 为何使用 IR

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-ir-taxonomy.md](./02-ir-taxonomy.md)

---

## 解耦与复用：m × n 架构

| 无 IR | 有 IR |
|-------|-------|
| 每种源语言 × 每种目标机 = **独立编译器** | **m 前端 + 1 IR + n 后端** |

```text
Lang A ──┐
Lang B ──┼──► IR ──┬──► x86
Lang C ──┘          ├──► ARM
                    └──► WASM …
```

**Rust**：`rustc` 前端 → **MIR**（Rust 自有 IR）→ **LLVM IR** → 多平台后端 — 典型 m×n 实例。

---

## 优化基础

- 不同 **IR 形态**强调不同程序特性（**数据流**、**控制流**、**嵌套结构**）。
- 优化器常需：
  - **多遍扫描**同一 IR，或
  - **IR 间转换**（如 AST → CFG → SSA）

| IR 特性 | 便于做的优化 |
|---------|--------------|
| **CFG + 基本块** | 块内/local 优化、循环识别 |
| **SSA** | 数据流分析、常量传播、DCE |
| **三地址码** | 寄存器分配、指令选择 |

→ ch8～10 · [04 LLVM optimize](../../../04_Learn-LLVM-17/part02_src_to_machine/chapter07_ir_optimize/)

---

## 与 CI 对照

| 体系 | 「IR」是什么 |
|------|--------------|
| **jlox** | **AST**（树遍历执行，无独立 lowering） |
| **clox** | **字节码 Chunk**（线性 + 栈 VM） |
| **rustc** | MIR + **LLVM IR**（工业级 SSA） |
