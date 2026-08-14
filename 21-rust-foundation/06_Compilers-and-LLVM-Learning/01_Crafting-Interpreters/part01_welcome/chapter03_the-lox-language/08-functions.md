# 第 3 章 · The Lox Language（Lox 语言概览） · §3.8 函数（Functions）

← [本章目录](./README.md) · 上一节：[07-control-flow.md](./07-control-flow.md) · 下一节：[09-classes.md](./09-classes.md)

---

| 特性 | 说明 |
|------|------|
| 声明 | `fun name(params) { ... }` |
| 返回 | `return expr;`；无 `return` 到末尾 → 隐式 **`nil`** |
| **一等公民** | 函数可作值：赋给变量、当参数传递 |
| **局部函数** | 函数内可再定义函数 |
| **闭包** | 内部函数**捕获**外部局部变量并保留 |

**实现预告**：Part II **ch10～11** · clox **ch24 Calling and Closures**。

**本仓库**：RFR 闭包与 `Fn` trait · `dyn` 分发 · 与 Lox 闭包语义对照。

---
