# 第 24 章 · Calling and Closures（调用与函数） · 本章定位

← [本章目录](./README.md) · 下一节：[01-calling-and-closures-upvalue.md](./01-calling-and-closures-upvalue.md)

---

此前 clox 仅在 **顶层脚本** 运行；本章起支持 **`fun`、调用、递归、return、原生函数**。函数体有 **独立 Chunk + CallFrame**；参数与局部变量共享 **同一块栈窗口**（零拷贝传递）。

| 对比 | jlox ch10 | clox ch24（本节） |
|------|-----------|-------------------|
| 函数值 | **`LoxFunction`** | **`ObjFunction`** + Chunk |
| 调用 | 新 Environment | **CallFrame 栈** |
| 参数 | define 在环境中 | **与 caller 压栈参数重叠** |
| return | **`Return` 异常** | **`OP_RETURN`** 弹帧 |
| native | **`LoxCallable`** | **`ObjNative`** |

| 小节 | 主题 |
|------|------|
| **§24.1～§24.2** | **`ObjFunction`** · Chunk · arity |
| **§24.3** | **`CallFrame`** 栈 |
| **§24.4～§24.5** | **`OP_CALL`** · 参数/局部 **栈重叠** |
| **§24.6** | **`return`** · **`OP_RETURN`** |
| **§24.7** | **`ObjNative`** · **`clock()`** |
| **闭包续** | 见 [ch25 Closures 笔记](../chapter25_objects/README.md)（原书 §24.8+ Upvalue） |

---
