# 第 29 章 · Superclasses（超类与继承） · §29.2 超类局部变量（A Superclass Local Variable）

← [本章目录](./README.md) · 上一节：[01-inheritance.md](./01-inheritance.md) · 下一节：[03-29-4-super.md](./03-29-4-super.md)

---

**问题**：子类方法内 **`super.cook()`** · 嵌套闭包也要解析 **`super`** → 不能仅靠栈帧临时查表。

**编译器**（类比 jlox ch13 §13.2）：

- 编译 **带超类的 class 方法** 时，在外层作用域 **`addLocal("super")`**。
- **`super`** 绑定为 **`ObjClass*`**（**超类对象** · 非实例）。

| 效果 | 说明 |
|------|------|
| **闭包 upvalue** | 可捕获 **`super` 类引用** |
| **静态 slot / upvalue 索引** | 与 **`this` slot 0** 并列的另一绑定 |

---
