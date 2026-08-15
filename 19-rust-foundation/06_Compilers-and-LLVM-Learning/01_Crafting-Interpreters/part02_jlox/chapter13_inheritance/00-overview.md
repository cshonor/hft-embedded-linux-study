# 第 13 章 · Inheritance（继承） · 本章定位

← [本章目录](./README.md) · 下一节：[01-superclasses-and-subclasses.md](./01-superclasses-and-subclasses.md)

---

**Part II 收官章**：补全 OOP 最后一块——**继承**与 **`super`**。至此 **jlox**（Java 树遍历解释器）功能完备；下一章起进入 **Part III · clox**（C 字节码 VM）。

| 对比 | ch12 | ch13 |
|------|------|------|
| 类关系 | 单类 | **子类 `<` 超类** |
| 方法查找 | 本类 + 实例字段 | **沿超类链** |
| 关键字 | `this` | **`super`** |

| 小节 | 主题 |
|------|------|
| **§13.1** | 超类 / 子类 · `<` 语法 |
| **§13.2** | 类内隐式 **`super`** 局部变量 |
| **§13.3** | **`super.method()`** · `Expr.Super` · Resolver 检查 |
| **§13.4** | **Conclusion** · jlox 完结 |

---
