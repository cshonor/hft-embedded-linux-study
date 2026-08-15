# 第 24 章 · Calling and Closures（调用与函数） · §24.1～§24.2 函数对象（Function Objects）

← [本章目录](./README.md) · 上一节：[01-calling-and-closures-upvalue.md](./01-calling-and-closures-upvalue.md) · 下一节：[03-call-frames.md](./03-call-frames.md)

---

**`ObjFunction`**（堆对象，ch19 `Obj` 继承）：

| 字段 | 说明 |
|------|------|
| **`arity`** | 形参个数（与 jlox/ch10 **255 上限** 一致） |
| **`name`** | **`ObjString*`** |
| **`chunk`** | 该函数 **专属字节码** |

**编译 `fun name(a, b) { ... }`**：

- 新建 **`Compiler`** 嵌套编译函数体 → 产出 **`function->chunk`**。
- **`OP_CLOSURE`**（闭包章节）或常量池 **`OBJ_VAL(function)`** + 全局/局部 define（随进度而定）。
- 顶层 **`fun`** → **`OP_DEFINE_GLOBAL`** 等。

**运行时**：函数是一等 **`Value`**（`OBJ_VAL`）。

---
