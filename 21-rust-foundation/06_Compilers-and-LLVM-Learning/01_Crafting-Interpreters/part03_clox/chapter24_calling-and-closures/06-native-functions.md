# 第 24 章 · Calling and Closures（调用与函数） · §24.7 原生函数（Native Functions）

← [本章目录](./README.md) · 上一节：[05-return-statements.md](./05-return-statements.md) · 下一节：[07-call.md](./07-call.md)

---

**`ObjNative`**：`Obj` + **C 函数指针** `NativeFn`。

```c
typedef Value (*NativeFunction)(int argCount, Value* args);
```

| 要点 | 说明 |
|------|------|
| **`OP_CALL`** | 与 **`ObjFunction`** 共用路径 |
| **`callValue`** | 判别类型 → **`callNative`** |
| **`clock()`** | 内置唯一 native · 返回秒级时间 |

**注册**：全局 **`defineNative("clock", fn)`**（实现细节随 REPL 引导）。

**对照 jlox ch10 §10.2**：同样为性能测试准备 **`clock()`**。

---
