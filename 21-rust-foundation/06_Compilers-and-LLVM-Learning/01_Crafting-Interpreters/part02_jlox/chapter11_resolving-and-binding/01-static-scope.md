# 第 11 章 · Resolving and Binding（解析与绑定） · §11.1 静态作用域（Static Scope）

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-semantic-analysis.md](./02-semantic-analysis.md)

---

**Lox 规则**：变量归属由**源码书写位置（词法结构）**决定，而非运行时动态栈。

**ch10 动态查找的问题**（示意）：

```lox
var a = "global";
{
  fun show() { print a; }
  var a = "block";
  show();  // 期望 "global" 还是 "block"？
}
```

- 若 `show` 闭包捕获的是 **Environment 引用**，块内 **`var a`** 可能在同一链上 **遮蔽** 全局 `a`，闭包读到 **block**。
- **词法作用域**要求：闭包内的 `a` 应绑定到 **定义 `show` 时可见的那个 `a`（global）**。

| 概念 | 说明 |
|------|------|
| **Lexical scoping** | 编译 / 分析阶段确定绑定 |
| **Dynamic scoping** | 运行时按调用栈找（Lox **不是**） |

---
