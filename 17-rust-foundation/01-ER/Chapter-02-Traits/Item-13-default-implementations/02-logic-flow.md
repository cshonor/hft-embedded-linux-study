# Item 13 · 逻辑脉络

← [Item 13 目录](./README.md)

```text
强制少量基元方法（如 next / len）
  → trait 内默认实现派生大量便捷 API
  → 实现者省力、使用者功能全
  → 库演进：新增带默认体的方法 ≈ 向后兼容
  → 具体类型可 override 更高效实现
```

### 向后兼容

- 已发布 trait **新增带 default body 的方法** → 旧 impl **不必改**，通常 **不破坏** API。
- 对比：新增**无默认**的必需方法 → 所有 impl 必须补实现 → **破坏性**。

### 可覆盖（Override）

- 默认实现是**后备**；若类型有 O(1) 捷径（如 `is_empty`），可 **override** 默认的 O(n) 逻辑。

---
