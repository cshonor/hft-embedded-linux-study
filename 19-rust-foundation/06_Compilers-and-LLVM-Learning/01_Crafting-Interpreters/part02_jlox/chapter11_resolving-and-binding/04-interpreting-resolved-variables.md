# 第 11 章 · Resolving and Binding（解析与绑定） · §11.4 解释已解析的变量（Interpreting Resolved Variables）

← [本章目录](./README.md) · 上一节：[03-a-resolver-class.md](./03-a-resolver-class.md) · 下一节：[05-resolution-errors.md](./05-resolution-errors.md)

---

运行时不再 **`environment.get(name)` 逐层向上搜**：

| API | 行为 |
|-----|------|
| **`getAt(distance, name)`** | 从当前 env 沿 `enclosing` 走 **distance** 步，再 **get** |
| **`assignAt(distance, name, value)`** | 同上，再 **assign** |

**`visitVariableExpr`**：

```text
if (locals.containsKey(expr))
  return environment.getAt(locals.get(expr), name);
else
  return globals.get(name);  // 顶层全局
```

**效果**：

- 闭包内变量 **固定** 到定义时那层 binding。
- 内层 **`var` 遮蔽** 不影响已解析的 **distance**。
- 性能：O(1) 直达，非 O(深度) 链式查找。

---
