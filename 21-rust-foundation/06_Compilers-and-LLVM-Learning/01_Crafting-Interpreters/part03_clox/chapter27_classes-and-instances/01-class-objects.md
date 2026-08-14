# 第 27 章 · Classes and Instances（类与实例） · 类对象（Class Objects）

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-instances.md](./02-instances.md)

---

```lox
class Pair {
  // 方法 ch28
}
```

**编译**：

```text
OP_CLASS  name_constant
→ runtime: ObjClass* 入常量 / 栈
→ OP_DEFINE_GLOBAL 等绑定类名
```

**`ObjClass`**（堆 `Obj` 子类型）：

| 字段 | 说明 |
|------|------|
| **`name`** | **`ObjString*`** |
| **`methods`** | **`Table`**（ch28 填方法） |

**类本身也是 Value**：可在运行时传递、赋值（与 jlox 一致）。

---
