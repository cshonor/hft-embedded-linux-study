# 第 11 章 · 指令筛选 · 本章定位

← [本章目录](./README.md) · 上一章：[ch10 标量优化](../chapter10_scalar-opt/README.md) · 下一节：[01-tree-walk.md](./01-tree-walk.md)

---

**Part IV 代码生成** 开始 — 跨越 IR 与物理硬件的鸿沟。

```text
Part III (ch8～10)   改进 IR（机器无关为主）
        ↓
Part IV (ch11～13)   IR → 目标机器码
        ↓
  ch11 选「用什么指令」
  ch12 排「指令什么顺序」
  ch13 分「值进哪个寄存器」
```

| ch1 三阶段 | 本章 |
|------------|------|
| **Back end 第一关** | [§4.1 指令筛选](../chapter01_overview/04-translation-pipeline-example.md) |

**核心问题**：同一 IR 操作往往对应**多种**合法机器指令组合 — 编译器须选**正确且高效**的一组。

**教学 ISA**：全书常用假想 **ILOC** 汇编；思想可迁移到 x86 / ARM / LLVM `SelectionDAG`。

---

## 与 ch10 分界

| ch10 | ch11 |
|------|------|
| IR 层标量 Pass | **目标 ISA** 层指令选择 |
| 机器无关为主 | **机器相关**为主 |

窥孔优化在 ch10 分类中属机器相关，**算法细节在本章**。
