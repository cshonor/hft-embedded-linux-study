# 第 12 章 · Classes（类） · §12.2 创建实例（Creating Instances）

← [本章目录](./README.md) · 上一节：[01-class-declarations.md](./01-class-declarations.md) · 下一节：[03-properties-on-instances.md](./03-properties-on-instances.md)

---

Lox **没有 `new` 关键字**：

```lox
var breakfast = Breakfast();  // 把类当 callable
```

| 步骤 | 说明 |
|------|------|
| **`LoxClass` 实现 `LoxCallable`** | `call()` 创建 **`LoxInstance`** |
|  arity | 通常 **0**（构造逻辑放 `init`） |
| 返回 | 新实例对象 |

**对照**：JavaScript 里 `new Foo()` vs Lox 直接 **`Foo()`** —— 语法更简，语义仍是通过类生成实例。

---
