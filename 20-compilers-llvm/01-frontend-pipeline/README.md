# 01 · 前端流水线：词法 → 语法 → AST → IR

**任务**：跑通编译器前端最小闭环。
**验收**：对一小段 C 子集源码，本工具能输出合法 `.ll`，且 `lli` 能正确运行。

对应课程 **1–11 讲**（映射表见 [`../_refs/c-compiler-llvm-course.md`](../_refs/c-compiler-llvm-course.md)）。

---

## 子任务

| # | 子任务 | 产出 | 状态 |
|:--:|--------|------|------|
| 1.1 | 环境与 LLVM 构建 | 构建脚本 + 踩坑记录 | 待做 |
| 1.2 | 词法分析器 | `lexer` + 单测 | 待做 |
| 1.3 | 语法分析器 + AST | `parser` + AST 定义 | 待做 |
| 1.4 | IR 生成 | `codegen`（IRBuilder） | 待做 |
| 1.5 | 变量与符号表 | 作用域管理 | 待做 |
| 1.6 | 控制流（if / for / break / continue） | 基本块与 CFG | 待做 |
| 1.7 | 阶段验收 | C 子集 → `.ll` → `lli` | 待做 |

---

## 三边互证（本任务的核心价值）

同一件事用三种材料各看一遍，前端直觉才真正落地：

| 材料 | 侧重点 |
|------|--------|
| 本目录（C++ / LLVM） | 工程实现、LLVM API |
| `17-rust-foundation/06/01_Crafting-Interpreters` | 前端直觉、树遍历解释器 |
| `17-rust-foundation/06/03_Build-Your-Own-Compiler` | C♭ 语法、JavaCC 生成 |

---

## 前置

- C++11/14 + STL（姊妹仓 `cpp-learning-notes` 01–06）
- LLVM 17 构建完成（子任务 1.1）

---

## HFT / 嵌入式关联

- **HFT**：做完本任务后，`rustc --emit asm` 产出的控制流、分支、内联结果你能自己读懂，不再靠猜。
- **嵌入式**：理解 AST → IR 的降级过程，是看懂交叉编译产物和 `-Os` 取舍的前提。
