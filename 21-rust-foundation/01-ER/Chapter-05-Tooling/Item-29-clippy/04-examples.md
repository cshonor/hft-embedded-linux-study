# Item 29 · 案例与代码

← [Item 29 目录](./README.md)

### 魔法数字 → 标准库常数

```rust
pub fn circle_area(radius: f64) -> f64 {
    let pi = 3.14;
    pi * radius * radius
}
```

Clippy：`approximate value of f{32,64}::consts::PI found`  
建议：改用 **`std::f64::consts::PI`**。

### 与 ER 条目联动示例

| Lint 方向 | 关联 Item |
|-----------|-----------|
| glob re-export / wildcard | Item 23 |
| `negative_feature_names` | Item 26 |
| `cast_possible_truncation` | Item 5 |
| `clone_on_copy` | Item 10 |
| `unwrap_or_default` 等 | Item 3 |

---
