# 第 12 章 · Classes（类） · §12.1 类声明（Class Declarations）

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-creating-instances.md](./02-creating-instances.md)

---

```lox
class Breakfast {
  cook() {
    print "Eggs!";
  }
}
```

- **`class Name { methods... }`** → **`Stmt.Class`**（name + methods 列表）。
- 执行：`define(name, new LoxClass(...))`，类名与 **`fun`** 一样进入环境。
- **`LoxClass`**：保存类名 + **方法名 → LoxFunction** 的映射。

---
