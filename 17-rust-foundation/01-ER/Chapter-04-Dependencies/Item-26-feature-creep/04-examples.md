# Item 26 · 案例与代码

← [Item 26 目录](./README.md)

### 破坏性：pub 字段 `#[cfg(feature)]`

```rust
#[derive(Debug)]
pub struct ExposedStruct {
    pub data: Vec<u8>,
    #[cfg(feature = "schema")]
    pub schema: String,
}
```

- 用户写 `ExposedStruct { data: vec![] }` 以为 OK。
- 深层依赖开启 `schema` → unification 打开 feature → **缺少字段 `schema`**，编译失败。

### 正面范例：`std` / `alloc` / optional 依赖

```toml
[features]
default = ["std"]
std = []
# 或 optional dep 对应 feature，no_std 下不启用
```

- Item 33：`no_std` 环境靠**不启** `std`/`alloc` feature，而非 `no_std` 否定 feature。

---
