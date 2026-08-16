# Item 26 · 核心知识点

← [Item 26 目录](./README.md)

### 条件编译（`cfg` / `cfg_attr`）

- 控制项/代码块是否进入最终产物。
- 作用于 **AST**，非 C 式行级预处理器。
- 配置：`test`、`panic = "abort"` 等；部分可多值同时为真（如 32/64 位 atomics）。

### Features（Cargo）

- 在 `cfg` 之上的**包级开关**，定义于 `Cargo.toml` **`[features]`**。
- 构建时选择性编译 crate 部分功能。

### 隐式 feature

```toml
[dependencies]
serde = { version = "1", optional = true }
# 自动创建 feature "serde"
```

### Feature 统一（与 Item 25）

- 依赖图多处请求**同版本** crate 的不同 features → Cargo **取并集**，只构建一次。

---
