# 第 25 章 · Closures（闭包） · Upvalue（上值）

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-flattening-upvalues.md](./02-flattening-upvalues.md)

---

**问题**：内层函数引用外层 **局部变量**（在 caller 的 stack slot 上）；外层 **`OP_RETURN` 弹帧** 后 slot 失效。

**Lua 式解法**：**Upvalue** = 指向「变量所在位置」的 **间接层**，位置可以是 **栈槽** 或 **堆上 Upvalue 对象**。

| 类型 | 说明 |
|------|------|
| **`ObjUpvalue`** | 堆对象；**`location`** 指向 `Value*`（栈或自身 closed 字段） |
| **`ObjClosure`** | 包装 **`ObjFunction*`** + **`Upvalue* upvalues[]`** |
| 编译 | 被捕获的 local 标 **`isCaptured`**；emit **`OP_CLOSURE`** + upvalue 描述 |

**运行时读取**：

| 指令 | 行为 |
|------|------|
| **`OP_GET_UPVALUE i`** | 经 closure.upvalues[i]->location 读 Value |
| **`OP_SET_UPVALUE i`** | 写入同一 location |

**对照 jlox [ch11](../../part02_jlox/chapter11_resolving-and-binding/README.md)**：`getAt(distance)` 静态绑定；clox 用 **upvalue 索引** 在字节码里固定。

---
