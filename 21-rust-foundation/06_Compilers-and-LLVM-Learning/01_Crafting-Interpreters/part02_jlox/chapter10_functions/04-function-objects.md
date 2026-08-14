# 第 10 章 · Functions（函数） · §10.4 函数对象（Function Objects）

← [本章目录](./README.md) · 上一节：[03-function-declarations.md](./03-function-declarations.md) · 下一节：[05-return-statements.md](./05-return-statements.md)

---

**`LoxFunction`** 表示 Lox 中的一等函数值：

| 字段 / 行为 | 说明 |
|-------------|------|
| 闭包环境 | 声明时所在的 **`Environment`**（§10.6 完善） |
| `call()` | **新建子 Environment**，`enclosing` 指向闭包环境 |
| 参数 | 在子环境中 **define** 每个形参 |
| body | 在新环境中执行 block |

**为何每次调用新建环境？**

- 递归 / 多次调用时，局部变量与参数**互不干扰**。
- 对照 ch8 **环境链**：函数体 = 新的块作用域，但 **enclosing 是定义处环境**（闭包），不是调用处。

```text
call foo():
  new env(enclosing = foo.closure)
    define params...
    execute body
```

---
