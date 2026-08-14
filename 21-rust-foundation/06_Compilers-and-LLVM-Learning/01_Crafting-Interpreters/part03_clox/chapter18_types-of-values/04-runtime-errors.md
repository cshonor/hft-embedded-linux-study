# 第 18 章 · Types of Values（值的类型） · 运行时错误（Runtime Errors）

← [本章目录](./README.md) · 上一节：[03-equality-and-comparison.md](./03-equality-and-comparison.md) · 下一节：---

多类型后，**`-false`**、**非 number 相加** 等需拦截。

| 机制 | 说明 |
|------|------|
| **`checkNumber(value)`** | 非 number → **`runtimeError("Operands must be numbers.")`** |
| **行号** | 用 ch14 **`lines[ip]`** |
| **中止** | **`INTERPRET_RUNTIME_ERROR`** · 不崩溃进程 |

**对照 jlox**：自定义 **`RuntimeError`** 异常 + 行号；clox **C 函数 + 返回码**。

**栈上类型错误**：在 **`OP_ADD`** 等执行前检查 **两个 pop 值**。

---
