# 第 8 章 · Statements and State（语句与状态） · §8.3 环境（Environments）

← [本章目录](./README.md) · 上一节：[02-variable-declarations.md](./02-variable-declarations.md) · 下一节：[04-assignment.md](./04-assignment.md)

---

**`Environment`**：核心状态容器。

```text
HashMap<String, Object>  // 变量名 → Lox 值
```

| 操作 | 行为 |
|------|------|
| **define(name, value)** | 在当前环境**定义**（顶层允许重复 define 的语义见书） |
| **get(name)** | **查找**变量值 |

Interpreter 持有 **current Environment**。

**本仓库**：RFR 第 1 章作用域 · 与 Rust 所有权/绑定对比（Lox 为动态 + 链式环境）。

---
