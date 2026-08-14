# 第 24 章 · Calling and Closures（调用与函数） · §24.6 返回语句（Return Statements）

← [本章目录](./README.md) · 上一节：[04-function-calls.md](./04-function-calls.md) · 下一节：[06-native-functions.md](./06-native-functions.md)

---

**`return expr;` / `return;`** → compile 值（或 nil）→ **`OP_RETURN`**。

**VM 执行 `OP_RETURN`**：

```text
result = pop()                    // 返回值
frameCount--                      // 弹帧
if frameCount == 0: 解释结束，result 留栈或打印
else:
  丢弃 callee..locals 栈段
  push(result) 给 caller
  ip 恢复 caller frame
```

| 对比 jlox ch10 §10.5 | clox |
|----------------------|------|
| **`throw Return`** 跳出 Visitor | **显式弹 CallFrame** |
| 无栈帧概念 | **结构化帧栈** |

---
