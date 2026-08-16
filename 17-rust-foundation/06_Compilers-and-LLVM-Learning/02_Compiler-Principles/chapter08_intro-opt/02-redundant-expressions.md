# 第 8 章 · 代码优化概述 · §2 消除冗余表达式

← [本章目录](./README.md) · 上一节：[01-scope-of-optimization.md](./01-scope-of-optimization.md) · 下一节：[03-beyond-basic-blocks.md](./03-beyond-basic-blocks.md)

---

优化核心目标之一：**别让机器重复算同样的东西**（在仍保持 [ch1 正确性](../chapter01_overview/02-two-fundamental-principles.md) 的前提下）。

---

## 有向无环图（DAG）

在**基本块**内，用 **DAG** 表示表达式：

| 特点 | 作用 |
|------|------|
| **共享子表达式节点** | 同一子式只出现一次 |
| **直观 CSE** | 公共子表达式消除（Common Subexpression Elimination） |

→ [ch5 §3 DAG vs AST](../chapter05_ir/03-graphical-ir.md)

块内重写 IR：第二次 `a+b` → 复用第一次结果临时名。

---

## 值编号（Value Numbering, VN）

| 思想 | 给每个**计算产生的值**分配唯一**编号** |
|------|----------------------------------------|
| **相同编号** | 两表达式**此时**必同值 → 可删冗余 |
| **经典且强大** | 块内 VN 是局部优化的 workhorse |

```text
t1 = a + b    →  VN(t1) = #1
t2 = a + b    →  VN(t2) = #1  →  t2 := t1
```

**安全条件**：`a`,`b` 在两次计算间**未被重新定义** — 块内常易保证。

---

## 局部方法定位

| | |
|---|---|
| **作用域** | Local（单基本块） |
| **复杂度** | 低 |
| **LLVM 类比** | Early CSE、块内简化 |

---

## 与 ch7

若 [ch7 lowering](../chapter07_code-shape/02-translating-operators.md) 生成重复 `load`/`add`，VN/DAG 在块内合并 — **形态 + 优化** 配合。
