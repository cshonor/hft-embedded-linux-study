# 第 8 章 · Statements and State（语句与状态） · §8.5 块作用域（Block Scope）

← [本章目录](./README.md) · 上一节：[04-assignment.md](./04-assignment.md) · 下一节：---

- **`{ ... }`** 创建**局部词法作用域**。
- **Shadowing**：内层 `var x` 可遮蔽外层同名 `x`。

### 环境链（Environment Chain）

```text
inner Environment  ──enclosing──▶  outer Environment  ──▶ … ──▶  global
```

| 时机 | 操作 |
|------|------|
| 进入 block | **新建** Environment，**enclosing** 指向当前环境，推入链 |
| 离开 block | **弹出**，恢复外层 |

- **lookup**：当前 env 没有 → 沿 **enclosing** 向上找。

**clox 预告**：局部变量用**栈槽 + 编译期深度**，不用 Java HashMap（ch22）。

---
