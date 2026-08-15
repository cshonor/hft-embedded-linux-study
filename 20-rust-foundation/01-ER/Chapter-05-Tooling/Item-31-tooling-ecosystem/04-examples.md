# Item 31 · 案例与代码

← [Item 31 目录](./README.md)

### 全书高频工具速查

| 领域 | 工具 | ER |
|------|------|-----|
| **unsafe / UB** | **Miri** | Item 16 |
| **依赖升级** | **Dependabot** / Renovate | Item 21 / 25 |
| **Semver break** | **cargo-semver-checks** | Item 21 |
| **未用依赖** | **cargo-udeps** | Item 25 |
| **CVE / license** | **cargo-deny** | Item 25 |
| **宏展开** | **cargo-expand** | Item 28 |
| **Lint** | **Clippy** | Item 29 |
| **汇编** | **Godbolt** | Item 30 |
| **Fuzz** | **cargo-fuzz** | Item 30 |
| **Bench** | **criterion** | Item 30 |
| **覆盖率** | **cargo-tarpaulin** / llvm-cov | Item 30 |
| **FFI 绑定** | **bindgen** | Item 35 |
| **Feature 穷举** | **cargo-hack** | Item 26 |

### `cargo metadata` 示例

```bash
cargo metadata --format-version 1 | jq '.packages[].name'
```

第三方工具读此 JSON，无需解析 `Cargo.toml` 文本。

### `cargo-expand`

```bash
cargo install cargo-expand
cargo expand --lib my_module::some_macro_user
```

调试 Item 28 复杂宏时必备。

---
