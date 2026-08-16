# 第 12 章 · Classes（类） · §12.6 构造函数与初始化器（Constructors and Initializers）

← [本章目录](./README.md) · 上一节：[05-this.md](./05-this.md) · 下一节：[07-ch12-vs-ch13.md](./07-ch12-vs-ch13.md)

---

约定：实例化后若类定义了 **`init`**，**自动调用**。

```lox
class Foo {
  init(a) {
    this.a = a;
  }
}
var f = Foo(1);
```

| 规则 | 说明 |
|------|------|
| 自动调用 | **`LoxClass.call()`** 创建实例后查找 **`init`** 并 invoke |
| **`return;` in init** | 隐式 **返回 `this`**（实例） |
| **禁止** | **`init` 中 `return expr;`**（expr 非空）→ Resolver / 运行时限制 |
|  arity | **`init`** 参数个数决定 **`Foo(...)` 所需参数** |

防止用户把 **`init`** 写成普通函数返回任意值，破坏「构造总是得到实例」的不变式。

---
