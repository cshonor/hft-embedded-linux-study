# 第 25 章 · Closures（闭包） · 本章定位

← [本章目录](./README.md) · 下一节：[01-calling-and-closures-upvalue.md](./01-calling-and-closures-upvalue.md)

---

jlox 闭包靠 **Java GC + Environment 捕获**；clox 局部变量在 **栈槽** 上，函数返回即 **弹栈销毁** → 闭包若仍指向栈地址 = **悬空指针**。

| 对比 | jlox ch10～11 | clox 闭包 |
|------|---------------|-----------|
| 捕获 | `LoxFunction.closure` Environment | **`ObjUpvalue*`** 间接层 |
| 生命周期 | GC 管 Environment | **`OP_CLOSE_UPVALUE`** 迁堆 |
| 编译 | Resolver **distance** | **`resolveUpvalue`** · **`OP_GET/SET_UPVALUE`** |

| 主题 | 要点 |
|------|------|
| **Upvalue** | 闭包捕获变量的 **间接引用** |
| **Flattening** | 嵌套函数 **逐层传递** upvalue |
| **Closing** | 作用域结束 · 值 **栈 → 堆** |

---
