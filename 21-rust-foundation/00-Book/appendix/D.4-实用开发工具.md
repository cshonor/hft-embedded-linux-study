# 附录 D：实用开发工具

> **The Book** · [Appendix D（英文）](https://doc.rust-lang.org/book/appendix-04-useful-development-tools.html)

Rust 安装包自带或官方推荐的日常工具。

---

## 1. rustfmt — 统一格式

```bash
cargo fmt              # 格式化整个 workspace
cargo fmt -- --check   # CI：仅检查不修改
```

- 遵循社区 [Rust Style Guide](https://doc.rust-lang.org/nightly/style-guide/)。
- 本仓库 ER-demos CI：`er-study-ci` job 中 `cargo fmt --all --check`。

---

## 2. rustfix — 自动修警告

```bash
cargo fix
```

- 对**有明确修复建议**的编译器警告自动改代码（如 unused import）。
- 常与 edition 迁移、`cargo fix --edition` 一起用 → [E.5 Editions](./E.5-Editions.md)。

---

## 3. Clippy — Lint

```bash
cargo clippy
cargo clippy -- -D warnings   # 警告当错误（CI 常用）
```

- 捕获常见 mistake、性能与风格问题。
- 本仓库：[ER Item 29](../../01-ER/Chapter-05-Tooling/Item-29-clippy/README.md)、[`ER-demos/clippy.toml`](../../01-ER/ER-demos/clippy.toml)。

---

## 4. rust-analyzer — IDE / LSP

- [rust-analyzer](https://rust-analyzer.github.io/)：补全、跳转定义、内联错误、类型提示。
- VS Code：安装 **rust-analyzer** 扩展（不要用旧版 Rust 扩展）。
- 本仓库配置：[docs/rust-analyzer-VSCode配置.md](../../docs/rust-analyzer-VSCode配置.md)、[`.vscode/settings.json`](../../.vscode/settings.json)。

---

## 5. 其他常用（正文分散）

| 工具 | 用途 | 章节 |
|------|------|------|
| `cargo doc` | 生成 / 打开文档 | [14.2](../14-cargo-crates/14.2-将crate发布到Crates.io.md) |
| `cargo test` | 测试 | [11 章](../11-testing/) |
| `cargo publish` | 发布 crate | [14.2](../14-cargo-crates/14.2-将crate发布到Crates.io.md) |
| Miri | UB 检测 | [ER Item 16 demo](../../01-ER/Chapter-03-Concepts/Item-16-avoid-unsafe/demo/) |

---

## 记忆卡片

| 工具 | 一句 |
|------|------|
| fmt | 格式统一 |
| clippy | Lint 提质量 |
| rust-analyzer | IDE 语言服务 |
| fix | 自动修编译警告 |
