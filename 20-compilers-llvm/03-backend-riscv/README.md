# 03 · 自研 RISC-V 后端 ❄️ 冻结

> **状态：冻结。默认不动。**
> 对应课程 **C 段：讲 38–61**（24 讲 / 视频 **17.6 h**，实际约 26–35 h）
> —— 映射表见 [`../_refs/c-compiler-llvm-course.md`](../_refs/c-compiler-llvm-course.md)

## 为什么冻结

| 理由 | 说明 |
|------|------|
| **目标平台错位** | 课程第 **45 讲「RISCV 工具链和模拟器」**确认目标平台是 **RISC-V**；主线是 HFT + **aarch64**（Pi 5）。后端知识（TableGen `.td`、指令选择 DAG、寄存器分配）跨 target 可迁移度**很低** |
| 时间成本最高段位 | 24 讲 / 17.6 h 视频，是全课最陡的一段，且几乎全部是 API 细节而非通用原理 |
| 已有同类判定 | `17-rust-foundation/06/04_Learn-LLVM-17/Learn-LLVM-17-学习取舍.md` 已把 TableGen、指令选择、后端扩展判为「**整段跳过**」，并已据此删除空壳章节 |
| 收益可延后 | 若哪天真要碰后端（交叉编译 target、软浮点、调用约定），再解冻 |

---

## 解冻条件（满足任一再考虑）

- 目标平台切换到 **RISC-V**
- 需要为自研 DSL / 策略语言做**专用后端**
- 从事工具链工程师方向（`01` `02` 已完成且仍想深入）

---

## 解冻后要做的事（讲次 → 阶段）

| # | 阶段 | 讲次 | 关键概念 |
|:--:|------|:----:|----------|
| 3.1 | 后端框架 | 38–39 | `llc` 识别后端、`TargetMachine` |
| 3.2 | AsmInfo | 40–41 | `initAsmInfo`、MC 层基础 |
| 3.3 | 指令降级与栈帧 | 42, 48 | `TargetLowering`、`FrameLowering`、frameIndex 消除 |
| 3.4 | 指令选择 | 43, 46–47, 53 | `DAGToDAGISel`、memory operand、load/store |
| 3.5 | 汇编输出 | 44 | `AsmPrinter`、MC 层 |
| 3.6 | 平台与联调 | 45, 59 | RISC-V 工具链与模拟器、八皇后前后端联调 |
| 3.7 | 调用约定与浮点 | 49–52, 61 | 参数传递、软浮点库调用 |
| 3.8 | 全局与类型 | 54, 57–58 | 全局变量 lowering、类型 legalization |
| 3.9 | 控制流与优化 | 55–56, 60 | 分支指令、call 匹配优化 |
| 3.10 | 验收 | — | `.s` 经 riscv 工具链汇编链接并运行 |

---

## 冻结期间的替代资源

- 想懂**通用**后端原理（不看具体 target）→ `17-rust-foundation/06/02_Compiler-Principles`（《编译器工程》橡书）第 11–13 章
- 想看 **aarch64 真实后端产物** → `17-rust-foundation/06/05_rustc-pipeline`（`rustc --emit asm`），这才是主线平台
- 想读后端源码 → LLVM `lib/Target/AArch64`（不是 `RISCV`）