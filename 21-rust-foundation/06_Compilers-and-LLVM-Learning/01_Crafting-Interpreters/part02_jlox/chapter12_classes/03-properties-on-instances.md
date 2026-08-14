# 第 12 章 · Classes（类） · §12.3 实例上的属性（Properties on Instances）

← [本章目录](./README.md) · 上一节：[02-creating-instances.md](./02-creating-instances.md) · 下一节：[04-methods-on-classes.md](./04-methods-on-classes.md)

---

**`.` 语法**：

```lox
instance.field      // Get
instance.field = v  // Set
```

| AST | 执行 |
|-----|------|
| **`Expr.Get`** | object 求值 → 在 **`LoxInstance`** 上查字段 |
| **`Expr.Set`** | 先求 object 与 value → **set** 字段 |

**`LoxInstance`**：内部 **`Map<String, Object> fields`**，可 **动态增删** 字段（类似 JS 对象扩展）。

**查找顺序（Get）**（§12.4 细化）：

1. 实例 **fields**
2. 否则类 **methods**

---
