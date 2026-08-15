# Effective Rust — Demo 索引

拓展全文：[ER-拓展索引.md](../ER-拓展索引.md) · Workspace 配置：[WORKSPACE.md](./WORKSPACE.md) · 目录约定：[目录结构.md](../目录结构.md) · Rust 工具链：[README § 工具链](../../README.md#rust-工具链stablenightly--edition)

## 运行

```bash
cd 01-ER
cargo test --workspace
cargo run -p item-05-06-newtype
cd Chapter-04-Dependencies/Item-24-re-export-api-types/demo && cargo run -p consumer-newtype
cd Chapter-06-Beyond-Standard-Rust/Item-35-bindgen/demo-sys-workspace && cargo test --workspace
```

各 Item 的笔记与 demo 同目录，见 [目录结构.md](../目录结构.md)。

## Item → Demo（按 Item 目录）

| Item | 路径 |
|------|------|
| 3 | [Item-03/demo](../Chapter-01-Types/Item-03-option-result-transforms/demo/) |
| 4 | [Item-04/demo](../Chapter-01-Types/Item-04-idiomatic-error-types/demo/) · [demo-core-error](../Chapter-01-Types/Item-04-idiomatic-error-types/demo-core-error/) |
| 5–6 | [Item-05/demo](../Chapter-01-Types/Item-05-type-conversions/demo/) |
| 15 | [Item-15/demo](../Chapter-03-Concepts/Item-15-borrow-checker/demo/) |
| 16 | [Item-16/demo](../Chapter-03-Concepts/Item-16-avoid-unsafe/demo/) + CI `miri` |
| 18 | [Item-18/demo](../Chapter-03-Concepts/Item-18-dont-panic/demo/) |
| 20 | [Item-20/demo](../Chapter-03-Concepts/Item-20-avoid-over-optimize/demo/) |
| 21 | [WORKSPACE.md](./WORKSPACE.md) MSRV + CI `msrv` / `semver-checks` |
| 22 | [Item-22/demo](../Chapter-04-Dependencies/Item-22-minimize-visibility/demo/) |
| 24 | [Item-24/demo](../Chapter-04-Dependencies/Item-24-re-export-api-types/demo/) |
| 25 | [Cargo.toml](./Cargo.toml) + [deny.toml](../../deny.toml) + Dependabot |
| 26 | [Item-26/demo](../Chapter-04-Dependencies/Item-26-feature-creep/demo/) |
| 29 | [clippy.toml](./clippy.toml) |
| 30 | [Item-30/demo](../Chapter-05-Tooling/Item-30-beyond-unit-tests/demo/) + CI `matrix-demo` |
| 32 | [er-study-ci.yml](../../.github/workflows/er-study-ci.yml) |
| 33 | [Item-33/demo](../Chapter-06-Beyond-Standard-Rust/Item-33-no-std/demo/) |
| 34 | [Item-34/demo](../Chapter-06-Beyond-Standard-Rust/Item-34-ffi-boundaries/demo/) |
| 35 | [demo-bindgen](../Chapter-06-Beyond-Standard-Rust/Item-35-bindgen/demo-bindgen/) · [demo-sys-workspace](../Chapter-06-Beyond-Standard-Rust/Item-35-bindgen/demo-sys-workspace/) |

其余 Item 见拓展索引中的 Book demo 链接。
