# 第 29 章 · Superclasses（超类与继承） · 本章定位

← [本章目录](./README.md) · 下一节：[01-inheritance.md](./01-inheritance.md)

---

OOP **最后一块**：**`<` 继承** · **copy-down 方法表** · **`super`** 静态绑定 · **`OP_SUPER_INVOKE`**。

| 对比 | jlox ch13 | clox ch29 |
|------|-----------|-----------|
| 语法 | **`class Sub < Super`** | 同 |
| 方法查找 | **超类链** 动态 | **子类表已含拷贝** · 一次 hash |
| **`super`** | **`Expr.Super`** · Resolver | **`OP_GET_SUPER`** · 编译期 **`super` 局部** |
| 优化 | — | **`OP_SUPER_INVOKE`** |

| 小节 | 主题 |
|------|------|
| **§29.1** | **`<` 继承** · **`OP_INHERIT`** · copy-down |
| **§29.2** | 隐式 **`super`** 局部 |
| **§29.3～§29.4** | **`OP_GET_SUPER`** · **`OP_SUPER_INVOKE`** |

---
