# Item 19 · 核心知识点

← [Item 19 目录](./README.md)

### 反射（Reflection）

- 运行时检查/修改自身结构：查类型、读写字段、调方法。
- Python / Ruby / Java / Go 等常见；**Rust 无完整运行时反射**，也无 C++ 式 **`typeid` / `dynamic_cast` RTTI**。

### `std::any`：有限替代

| API | 作用 |
|-----|------|
| **`type_name::<T>()` / `type_name_of_val`** | 编译期类型名 → **调试/诊断**；非运行时探测 |
| **`TypeId`** | 类型全局唯一 ID → **逻辑里做唯一性校验** |
| **`Any` trait** | 装箱为 `dyn Any`，运行时 **`is` / `downcast_ref` / `downcast_mut`** |

---
