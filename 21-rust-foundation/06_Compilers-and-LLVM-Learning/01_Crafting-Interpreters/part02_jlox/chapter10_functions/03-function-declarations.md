# 第 10 章 · Functions（函数） · §10.3 函数声明（Function Declarations）

← [本章目录](./README.md) · 上一节：[02-native-functions.md](./02-native-functions.md) · 下一节：[04-function-objects.md](./04-function-objects.md)

---

```lox
fun name(param1, param2) {
  // body
}
```

- **`fun`** 关键字 → 函数名 · 参数列表 · **`{}` 块** 为 body。
- **AST**：`Stmt.Function`（name, params, body block）。
- **执行**：在**当前环境**中 `define(name, LoxFunction)`，函数名与变量一样可被后续引用。

---
