# 第 9 章 · Control Flow（控制流） · §9.5 For 循环（For Loops）

← [本章目录](./README.md) · 上一节：[03-while-loops.md](./03-while-loops.md) · 下一节：[05-ch7-9.md](./05-ch7-9.md)

---

C 风格三件套：

```lox
for (init; cond; increment) {
  body
}
```

### 脱糖（Desugaring）

**后端不必实现专用 `for` 执行逻辑** —— Parser 在前端把 `for` **翻译成**已有 AST：

```text
{
  init;
  while (cond) {
    body;
    increment;
  }
}
```

| 优点 | 说明 |
|------|------|
| 解释器更简单 | 只实现 `while` + block |
| 经典编译器技巧 | 语法糖在前端消掉 |

**本仓库**：Rust `for` desugar 到 loop/iterator · 同一「前端脱糖」思想。

---
