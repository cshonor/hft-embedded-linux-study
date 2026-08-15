# 第 8 章 · 代码优化概述 · §4 全局冗余消除

← [本章目录](./README.md) · 上一节：[03-beyond-basic-blocks.md](./03-beyond-basic-blocks.md) · 下一节：[05-interprocedural-opt.md](./05-interprocedural-opt.md)

---

**Global** 作用域 — 整函数 CFG 上消除冗余。

---

## 可用表达式（Available Expressions）

| 问题 | 在某程序点 **p**，哪些表达式 **x op y** 已被计算且 **x,y 此后未变**？ |
|------|---------------------------------------------------------------------|
| **用途** | 若 `a+b` 已可用 → 后续 `a+b` **直接用**旧结果，不必重算 |
| **分析类型** | **正向**数据流 — ch9 详述方程与迭代 |

```text
BB1:  t1 = a + b
      …
BB2:  t2 = a + b   →  若可用 → t2 := t1
```

---

## 与值编号 / DAG 关系

| 技术 | 层次 |
|------|------|
| **块内 DAG/VN** | Local，不需全局迭代 |
| **可用表达式** | Global，需 **meet/over** CFG 传播 |

二者目标相同（消冗余），**机制**不同 — ch9 统一在数据流框架下。

---

## 其他全局冗余（预告 ch9～10）

| 分析 | 消什么 |
|------|--------|
| **到达定值（Reaching definitions）** | 常量传播、拷贝传播 |
| **活跃变量（Live variables）** | DCE |
| **SSA + GVN** | 现代编译器组合技 |

**LLVM**：GVN、EarlyCSE、LICM 等 Pass → [04 optimize_compare](../../../04_Learn-LLVM-17/ir_samples/optimize_compare/)

---

## HFT / Rust

- 热循环内重复 `load` 字段 → 可用表达式 + LICM → 少内存访问。
- **`rustc -C opt-level=3`**：全局 Pass 链在此类分析上运行。
