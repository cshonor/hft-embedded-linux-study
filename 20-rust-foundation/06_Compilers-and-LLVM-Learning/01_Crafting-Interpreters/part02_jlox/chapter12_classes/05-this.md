# 第 12 章 · Classes（类） · §12.5 This

← [本章目录](./README.md) · 上一节：[04-methods-on-classes.md](./04-methods-on-classes.md) · 下一节：[06-constructors-and-initializers.md](./06-constructors-and-initializers.md)

---

**`this`** 关键字：方法内指向 **当前实例**。

| 机制 | 说明 |
|------|------|
| 调用 bound method | 创建 **新 Environment**，**`define("this", instance)`** |
| 方法体 | 在该环境中执行，`this` 与参数、局部变量一样 lookup |
| **Resolver** | 进入 **class method** 时 **`resolveLocal(..., "this")`**；类外使用 **`this`** → **resolution error** |

**与闭包关系**：方法体仍是 **`LoxFunction`**，可捕获外层变量；**`this` 环境** 在 **每次调用** 时叠加在 closure 之上（或作为 innermost 层）。

```text
call boundMethod:
  env(enclosing = method.closure)
    this = instance
    params...
    execute body
```

---
