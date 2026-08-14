# 第 13 章 · Inheritance（继承） · §13.2 超类局部变量（A Superclass Local Variable）

← [本章目录](./README.md) · 上一节：[01-superclasses-and-subclasses.md](./01-superclasses-and-subclasses.md) · 下一节：[03-calling-superclass-methods.md](./03-calling-superclass-methods.md)

---

嵌套类 + 闭包场景下，**`super`** 必须像 **`this`** 一样有稳定的词法绑定。

| 机制 | 说明 |
|------|------|
| **Resolver 进入 class 方法** | 若类有超类，在当前作用域 **`declare("super")`** |
| 运行时 | 方法环境中 **`super`** 指向 **超类 `LoxClass`**（非实例） |
| 目的 | 闭包内 **`super`** 解析到正确层级，不被动态遮蔽搞乱 |

**对照 ch11**：静态绑定 + 局部名 **`super`**，与 **`this`** 同一套 Resolver 模式。

---
