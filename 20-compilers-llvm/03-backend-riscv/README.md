# 03 · 自研 RISC-V 后端 ❄️ 冻结

> **状态：冻结。默认不动。**

## 为什么冻结

| 理由 | 说明 |
|------|------|
| 与目标主线无关 | 主线是 HFT + **aarch64** 嵌入式（Pi 5），不是 RISC-V 工具链 |
| 时间成本极高 | 后端部分约占课程 24 讲 / 20+ 小时，是全课最陡的一段 |
| 已有同类判定 | `17-rust-foundation/06/04_Learn-LLVM-17/Learn-LLVM-17-学习取舍.md` 已把 TableGen、指令选择、后端扩展判为「**整段跳过**」 |
| 收益可延后 | 若哪天真要碰后端（交叉编译 target、软浮点、调用约定），再解冻 |

---

## 解冻条件（满足任一再考虑）

- 目标平台切换到 **RISC-V**
- 需要为自研 DSL / 策略语言做**专用后端**
- 从事工具链工程师方向（`01` `02` 已完成且仍想深入）

---

## 解冻后要做的事

| # | 阶段 | 关键概念 |
|:--:|------|----------|
| 3.1 | 后端框架 | `llc` 识别后端、`TargetMachine`、`AsmInfo` |
| 3.2 | 目标描述 | TableGen、`.td` 文件、寄存器与指令定义 |
| 3.3 | 指令降级 | `TargetLowering`、DAG、`DAGToDAGISel` |
| 3.4 | 栈帧 | `FrameLowering`、prologue / epilogue |
| 3.5 | 汇编输出 | `AsmPrinter`、MC 层 |
| 3.6 | 调用约定与软浮点 | 参数传递、浮点库调用 |
| 3.7 | 验收 | `.s` 经 riscv 工具链汇编链接并运行 |

---

## 冻结期间的资源

- 课程 38 讲之后内容 → 映射表 [`../_refs/c-compiler-llvm-course.md`](../_refs/c-compiler-llvm-course.md)（待补全）
- 理论对照 → `17-rust-foundation/06/02_Compiler-Principles`（《编译器工程》橡书）第 11–13 章
