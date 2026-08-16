# 第 8 章 · Statements and State（语句与状态） · §8.4 赋值（Assignment）

← [本章目录](./README.md) · 上一节：[03-environments.md](./03-environments.md) · 下一节：[05-block-scope.md](./05-block-scope.md)

---

- 语法：`identifier = expression`（**右结合**）。
- 左侧必须是 **l-value**（合法赋值目标），不能是任意表达式。

### 解析技巧

1. 先把左侧按**普通表达式**解析（如 `Variable` 节点）。
2. 若后面有 **`=`** → 验证左侧合法 → 转为 **`Expr.Assign`**（target + value）。

求值：先求 **value**，再写入 Environment。

---
