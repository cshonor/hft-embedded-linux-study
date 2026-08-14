# 第 11 章 · 指令筛选（Instruction Selection）

> **Engineering a Compiler 3e** · [02 编译器工程](../../README.md) · [本书目录](../../本书目录.md) · Part IV 代码生成

## 状态

- [x] 已读（笔记整理）

---

## 一句话

**Part IV 开篇 · 后端第一关** — 将 IR **映射**到目标 ISA：**树遍历**（直观基线）→ **树模式匹配 + 重写规则 + 最小代价铺盖（Tiling）** → **窥孔优化**；进阶 **学习窥孔模式 · 复杂指令序列生成** — 为 ch12 调度、ch13 寄存器分配铺路。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §1 | 简单树遍历方案 | [01-tree-walk.md](./01-tree-walk.md) |
| §2 | 树模式匹配 | [02-tree-pattern-matching.md](./02-tree-pattern-matching.md) |
| §3 | 窥孔优化 | [03-peephole-optimization.md](./03-peephole-optimization.md) |
| §4 | 高级话题 | [04-advanced-topics.md](./04-advanced-topics.md) |
| — | 速记 · 自测 |

---

## 与仓库其他部分

| 本书 ch11 | 对照 |
|----------|------|
| ch1 后端三难点 | [ch1 §4.1](../chapter01_overview/04-translation-pipeline-example.md) |
| ch10 机器相关优化 | [ch10 §1](../chapter10_scalar-opt/01-classification.md) — 窥孔属机器相关 |
| ch5 树形 IR | [ch5 §2](../chapter05_ir/02-linear-ir.md) · DAG |
| LLVM 后端 | [04 ch11 目标描述](../../../04_Learn-LLVM-17/part04_custom_backend/chapter11_target_desc/) · [ch12 指令选择](../../../04_Learn-LLVM-17/part04_custom_backend/chapter12_instr_select/) |
| RFR 寄存器 | [第 2 章寄存器/栈](../../../02-RFR/Chapter-02-Types/) — ch13 深化 |

---

## 逻辑脉络

树遍历基线 → 形式化模式匹配 → 窥孔修补 → 自动化与学习。

---

## 速记

## 本章速记

```text
§1  树遍历：一节点一指令，简单但难融合
§2  重写规则 + 最小代价 Tiling + BURG/TableGen
§3  窥孔：滑动小窗口，删冗余 move/load
§4  学习窥孔 · 宏序列（call/div/legalize）
```

---

## 三句背诵

1. **指令筛选 = IR 树 → 目标 ISA 指令（常多对一）。**
2. **现代方案：模式匹配铺盖整树，窥孔局部修补。**
3. **选完指令还要调度、分寄存器 — 后端三关才完。**

---

## 方法对照

| 方法 | 视野 | 典型用途 |
|------|------|----------|
| 树 walk | 单节点 | 教学、极简后端 |
| 树模式匹配 | 整棵 IR 树 | LLVM SelectionDAG |
| 窥孔 | 2～5 条指令 | 删冗余、寻址合并 |

---

## 自测

- [ ] 树 walk 与 Tiling 的核心区别
- [ ] 重写规则、铺盖、代价模型各一句话
- [ ] 窥孔与全局优化的边界
- [ ] 后端三关顺序（本书 ch11→12→13）

---

## 阅读进度

- [x] ch11 指令筛选
- [x] ch12 指令调度

