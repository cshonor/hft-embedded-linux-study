# 第 10 章 · Functions（函数） · §10.2 原生函数（Native Functions）

← [本章目录](./README.md) · 上一节：[01-function-calls.md](./01-function-calls.md) · 下一节：[03-function-declarations.md](./03-function-declarations.md)

---

Lox 需要与宿主（Java）交互 → **内置原生函数**，由 Java 实现、注册到全局环境。

| 要点 | 说明 |
|------|------|
| 实现方式 | 匿名类实现 **`LoxCallable`**（或 lambda） |
| 本书唯一内置 | **`clock()`** → 返回秒级运行时间（`System.currentTimeMillis()` 等） |
| 用途 | 后续性能对比、基准测试 |

全局 **`Environment`** 中 `define("clock", …)`，与普通 `LoxFunction` 同样通过 **`LoxCallable`** 调用。

---
