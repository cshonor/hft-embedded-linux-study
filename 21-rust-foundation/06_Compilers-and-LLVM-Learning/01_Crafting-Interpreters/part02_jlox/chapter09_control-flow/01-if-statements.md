# 第 9 章 · Control Flow（控制流） · §9.2 条件执行（If Statements）

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-logical-operators.md](./02-logical-operators.md)

---

- 实现 **`if (cond) … else …`**。
- **`Stmt.If`**：condition + thenBranch + optional elseBranch。

### 悬挂 else（Dangling else）

```lox
if (first) if (second) whenTrue; else whenFalse;
```

`else` 应绑定**最近的未闭合 `if`**（`second`），而非 `first`。

**解析策略**：**贪婪匹配** —— 见到 `else` 总是挂到**刚解析完的内层 `if`** 上（递归下降自然实现）。

**执行**：求 condition 的 **truthy** → 执行 then 或 else 分支（Stmt visitor）。

---
