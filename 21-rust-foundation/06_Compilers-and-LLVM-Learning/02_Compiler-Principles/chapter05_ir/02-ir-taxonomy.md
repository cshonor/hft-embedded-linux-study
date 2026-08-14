# 第 5 章 · 中间表示 · §2 IR 分类法

← [本章目录](./README.md) · 上一节：[01-why-ir.md](./01-why-ir.md) · 下一节：[03-graphical-ir.md](./03-graphical-ir.md)

---

按**结构组织方式**三大类：

| 类型 | 特点 | 直觉 |
|------|------|------|
| **图示 IR（Graphical）** | 知识编码在**图** — 节点、边、树 | 显式结构关系 |
| **线性 IR（Linear）** | 抽象机**伪代码**，严格顺序 | 接近机器码 |
| **混合型（Hybrid）** | 图 + 线结合 | 工程最常见 |

---

## 混合型（典型）

```text
        ┌──► BB2 ──┐
  BB1 ──┤          ├──► BB4
        └──► BB3 ──┘

  BB2 内部：线性三地址指令序列
```

- **块间**：**CFG**（控制流图）
- **块内**：**线性**指令

**LLVM IR**、多数优化编译器 ≈ Hybrid。

---

## 选型 Trade-off

| 偏图 | 偏线 |
|------|------|
| 保留嵌套、易自顶向下思考 | 易模拟执行、易匹配后端 |
| AST 上优化较 awkward | 栈机/三地址直接喂 VM 或 codegen |

→ [ch1 Trade-offs](../chapter01_overview/05-desired-properties.md)

---

## 本章后续

- [§3 图示 IR](./03-graphical-ir.md) — AST · DAG · CFG
- [§4 线性 IR](./04-linear-ir.md) — 栈机 · 三地址
