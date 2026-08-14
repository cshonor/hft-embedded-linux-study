# Item 19 · 重点结论

← [Item 19 目录](./README.md)

### 默认：别做反射架构

- Rust **本来就不支持** —— 最容易遵守的 Item 之一。
- 从 Java/Python 迁来别硬搬「运行时类型表」。

### 替代方案（优先级）

| 需求 | 方向 |
|------|------|
| 行为多态 | **Trait** + `impl` / `dyn Trait` |
| 无法写成方法的约束 | **标记 trait**（marker） |
| 按结构批量 codegen | **derive 宏**（Item 28），编译期生成 |
| 极少数异构集合 | **`Box<dyn Any>`** + downcast（逃生舱） |

### `dyn Any` 使用边界

- 仅当**必须擦除具体类型**且无法用 enum/trait 表达时。
- downcast **只能回到装箱时的原始类型**；不能探查「还实现了哪些 trait」。

---
