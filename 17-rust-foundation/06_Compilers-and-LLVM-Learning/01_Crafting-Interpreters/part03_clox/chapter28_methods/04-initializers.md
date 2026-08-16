# 第 28 章 · Methods and Initializers（方法与初始化器） · §28.4 初始化器（Initializers）

← [本章目录](./README.md) · 上一节：[03-this.md](./03-this.md) · 下一节：[05-optimized-invocations.md](./05-optimized-invocations.md)

---

**`init`**：实例化时 **自动 invoke**（在 **`ObjClass` + `OP_CALL`** 路径上检测 **`init` 方法**）。

| 规则 | 说明 |
|------|------|
| **自动调用** | `Klass()` 后若存在 **`"init"`** → **`OP_INVOKE` / call** |
| **隐式 `return this`** | **`init` 末尾** 编译 **`OP_GET_LOCAL 0` + `OP_RETURN`** |
| **禁止 `return expr`** | 编译期错误（expr 非空） |
| **arity** | **`init` 参数个数** = **`Klass(...)` 所需实参** |

**对照 jlox ch12 §12.6**：语义一致。

---
