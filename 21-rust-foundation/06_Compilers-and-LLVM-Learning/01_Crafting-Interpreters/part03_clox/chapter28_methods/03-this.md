# 第 28 章 · Methods and Initializers（方法与初始化器） · §28.3 This 关键字（This）

← [本章目录](./README.md) · 上一节：[02-bound-methods.md](./02-bound-methods.md) · 下一节：[04-initializers.md](./04-initializers.md)

---

**技巧**：**不单独 opcode** —— **`this` 是编译器合成的局部变量，固定占 slot 0**。

| 约定 | 说明 |
|------|------|
| **CallFrame slot 0** | 方法调用时 **receiver（实例）** 放在 **第一个局部槽** |
| 方法内 **`this`** | 编译为 **`OP_GET_LOCAL 0` / `OP_SET_LOCAL 0`** |
| 形参 | 从 **slot 1** 起 |

```text
OP_CALL method on instance:
  stack: [ ..., instance, arg1, arg2 ]
  frame.slots[0] = instance  → this
  frame.slots[1..] = params
```

| 优点 | 说明 |
|------|------|
| **复用局部变量机制** | 无 **`this` 专用指令** |
| **O(1)** | 与 **`GET_LOCAL`** 同速 |

**编译器**：进入方法体时 **`addLocal("this")`** 并 **`markInitialized`**（或等价固定 slot 0）。

**对照 jlox**：Resolver 限制 **`this`** 仅在方法内；clox 靠 **slot 0 约定 + 编译器**。

---
