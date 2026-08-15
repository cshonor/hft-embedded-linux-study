# Item 7 · 核心知识点

← [Item 7 目录](./README.md)

### 结构体初始化的刚性

- Rust 要求创建 struct 时**每个字段都有值** → 无未初始化内存。
- 多可选字段时，手写 `None` / 默认值 → **样板代码**多，字段增删易漏改。

### `Default` + 结构体更新语法的局限

```rust
let d = Details {
    name: "Bob".into(),
    ..Default::default()
};
```

- **前提**：所有字段类型都实现 `Default`。
- 某字段（如外部库的 `time::Date`）**没有合适默认值** → 此捷径失效。

### Builder 模式

- 单独 **`XxxBuilder`** 存构造中间状态；
- **setter** 逐步填字段；
- **`build()`** 产出最终 `Xxx`。

---
