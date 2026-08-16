# 第 10 章 · Functions（函数） · §10.6 局部函数与闭包（Local Functions and Closures）

← [本章目录](./README.md) · 上一节：[05-return-statements.md](./05-return-statements.md) · 下一节：[07-ch8-10.md](./07-ch8-10.md)

---

Lox 允许 **函数内定义函数**：

```lox
fun outer() {
  var x = "local";
  fun inner() {
    print x;  // 闭包：读 outer 的 x
  }
  return inner;
}
```

**闭包 (Closure)**：函数 + **其定义时的词法环境**。

| 机制 | 说明 |
|------|------|
| **`LoxFunction` 保存 `closure`** | 创建时记录当前 `Environment` |
| 调用 `inner` | 新环境的 `enclosing` = 保存的 closure，而非调用栈外层 |
| 结果 | 外层局部变量在 inner 执行时仍可见 |

**注意**：此章的「链式按名查找」在 **外层变量被重新声明** 时会有 Bug → **ch11 Resolver** 修复。

**对照**：

- **ch2** 语义分析阶段 · **ch11** 静态绑定 distance
- **Rust** 闭包捕获 · **LLVM** 仍用环境/帧，思路同源

---
