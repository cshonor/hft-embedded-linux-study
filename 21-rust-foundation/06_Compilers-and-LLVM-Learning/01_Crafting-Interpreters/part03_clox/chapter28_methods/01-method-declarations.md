# 第 28 章 · Methods and Initializers（方法与初始化器） · §28.1 方法声明（Method Declarations）

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-bound-methods.md](./02-bound-methods.md)

---

```lox
class Brunch {
  init(food) { this.food = food; }
  eat() { print this.food; }
}
```

**编译 `class` 体中的方法**：

- 每个方法 → 编译为 **`ObjFunction` + `ObjClosure`**（含 upvalue）。
- **`tableSet(&klass->methods, name, closure)`**。

| 结构 | 说明 |
|------|------|
| **`ObjClass.methods`** | **`Table`**：键 **`ObjString*`** 方法名 · 值 **`ObjClosure*`** |
| 实例字段 | 仍在 **`ObjInstance.fields`**（ch27）· **优先于同名方法** |

---
