# 第 11 章 · Resolving and Binding（解析与绑定） · 本章定位

← [本章目录](./README.md) · 下一节：[01-static-scope.md](./01-static-scope.md)

---

ch10 闭包靠 **环境链 + 按名向上查找** 能跑通多数情况，但存在 **词法作用域 Bug**（外层重新 `var` 后，闭包可能绑错变量）。ch11 引入 **语义分析 (Semantic Analysis)**：**运行前** 解析每个变量「在第几层环境」，运行时 **按 distance 直取**。

| 阶段 | ch4～10 | ch11 新增 |
|------|---------|-----------|
| 扫描 | Token | — |
| 解析 | AST | — |
| **分析** | — | **`Resolver`** 遍历 AST，写 binding |
| 执行 | Interpreter | **`getAt(distance)` / `assignAt(distance)`** |

| 小节 | 主题 |
|------|------|
| **§11.1** | **静态 / 词法作用域** · 动态查找缺陷 |
| **§11.2** | **语义分析** · 独立于执行 |
| **§11.3** | **`Resolver`** · 作用域栈 |
| **§11.4** | 解释已解析变量 · **distance** |
| **§11.5** | **Resolution Errors** · 如顶层 `return` |

---
