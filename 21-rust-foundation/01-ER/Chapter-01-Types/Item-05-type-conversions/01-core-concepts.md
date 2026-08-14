# Item 5 · 核心知识点

← [Item 5 目录](./README.md)

### 三类转换机制

| 类别 | 机制 | 示例 |
|------|------|------|
| **Manual（手动）** | 实现 `From` / `TryFrom` 等 trait | `MyType::from(x)` |
| **Semi-automatic（半自动）** | `as` 显式强转 | `x as u16` |
| **Automatic（自动 / Coercion）** | 编译器静默强制转换 | 见 §5 |

### 无隐式数值转换

- Rust **不在数值类型间做自动隐式转换**。
- 即使 `u32` → `u64` 看似「安全」，也要**显式** `as` 或 `From`/`Into`（若已实现）。

### 用户定义转换 trait

| Trait | 语义 | 返回值 |
|-------|------|--------|
| **`From<T>` / `Into<U>`** | 必然成功 | 直接得到目标类型 |
| **`TryFrom<T>` / `TryInto<U>`** | 可能失败 | `Result<_, Error>` |

### 解引用强制转换（Deref Coercion）

- 实现 **`Deref` / `DerefMut`** 的智能指针类型上，编译器可把 `&SmartPtr` **自动**转为 `&Target`（见 Book [15.2](../../../00-Book/15-smart-pointers/15.2-通过Deref将智能指针当作引用.md) · [15.2.1 嵌套/坑点](../../../00-Book/15-smart-pointers/15.2.1-Deref嵌套可变与编译坑.md)）。

---
