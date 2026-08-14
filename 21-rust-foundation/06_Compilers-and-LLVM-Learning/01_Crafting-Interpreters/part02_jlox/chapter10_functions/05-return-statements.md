# 第 10 章 · Functions（函数） · §10.5 返回语句（Return Statements）

← [本章目录](./README.md) · 上一节：[04-function-objects.md](./04-function-objects.md) · 下一节：[06-local-functions-and-closures.md](./06-local-functions-and-closures.md)

---

- 语法：`return expr;` 或 `return;`（等价 `return nil;`）。
- **AST**：`Stmt.Return`（可选 value）。

**问题**：jlox 用 **Visitor 递归** 执行语句，嵌套很深时，用普通 `return` 只能跳出当前 Java 方法，无法「从任意深度立刻结束函数体」。

**解法**：**异常作为非本地控制流**

| 类 | 作用 |
|----|------|
| **`Return`**（自定义异常） | 携带返回值 `Object value` |
| 执行 `return` | `throw new Return(value)` |
| `LoxFunction.call()` | `try { execute body } catch (Return r) { return r.value; }` |

经典技巧：树遍历解释器里用异常模拟 **longjmp** / 结构化退出。

---
