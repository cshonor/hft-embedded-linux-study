# 第 12 章 · Classes（类） · 本章定位

← [本章目录](./README.md) · 下一节：[01-class-declarations.md](./01-class-declarations.md)

---

为 Lox 加入 **基于类的 OOP**：类声明、实例化、字段、方法、**`this`**、**`init`**。除 **继承（ch13）** 外，OOP 核心在此章齐备。

| 运行时对象 | 角色 |
|------------|------|
| **`LoxClass`** | 类对象（方法表 + 可调用工厂） |
| **`LoxInstance`** | 实例（字段 HashMap） |
| **`LoxBoundMethod`** | 已绑定 `this` 的方法 |

| 小节 | 主题 |
|------|------|
| **§12.1** | **`class`** 声明 · **`LoxClass`** |
| **§12.2** | 实例化 · 类即工厂（无 `new`） |
| **§12.3** | **`.`** Get / Set · 实例字段 |
| **§12.4** | 类方法 · **`LoxBoundMethod`** · 字段优先 |
| **§12.5** | **`this`** · 方法环境 · Resolver 检查 |
| **§12.6** | **`init`** · 构造 / 初始化器规则 |

---
