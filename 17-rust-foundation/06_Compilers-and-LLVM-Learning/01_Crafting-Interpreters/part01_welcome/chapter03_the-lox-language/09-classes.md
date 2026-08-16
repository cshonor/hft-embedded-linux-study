# 第 3 章 · The Lox Language（Lox 语言概览） · §3.9 类（Classes）

← [本章目录](./README.md) · 上一节：[08-functions.md](./08-functions.md) · 下一节：[10-the-standard-library.md](./10-the-standard-library.md)

---

**基于类**的 OOP（非 JavaScript 式原型）。

### 声明与实例化

| 项目 | Lox 做法 |
|------|----------|
| 类 | `class Name { ... }` |
| 方法 | 类体中**不写** `fun` |
| 构造实例 | **无 `new`** —— **像调用函数一样调用类** |

### 状态与初始化

- 可**动态**给实例加属性（字段）。
- 方法内 **`this`** → 当前实例。
- **`init()`** 若存在 → 创建实例时**自动**作为构造函数调用。

### 继承（Inheritance）

| 项目 | 说明 |
|------|------|
| 模型 | **单继承** |
| 语法 | `class Child < Parent { ... }` |
| 超类调用 | **`super`** 调用被覆盖的父类方法 |

**实现预告**：Part II **ch12～13** · clox **ch27～28**。

---
