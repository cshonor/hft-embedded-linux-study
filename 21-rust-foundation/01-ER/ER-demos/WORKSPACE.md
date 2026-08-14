# Item 21：MSRV 与 `rust-version`

Workspace 根 [`../Cargo.toml`](../Cargo.toml) 声明：

```toml
[workspace.package]
rust-version = "1.70"
```

CI 见 [er-study-ci.yml](../../../.github/workflows/er-study-ci.yml) 的 `msrv` job。

本地验证：

```bash
rustup toolchain install 1.70.0
cd 01-ER
cargo +1.70.0 check --workspace
```

## 本 workspace 的 toolchain 策略

Rust 工具链基础概念（`main` / Stable / Nightly / Edition）见仓库根目录 [README.md](../../README.md#rust-工具链stablenightly--edition)。

[`rust-toolchain.toml`](../rust-toolchain.toml) 钉 **Stable**（fmt / clippy / test / MSRV 日常开发）：

```toml
[toolchain]
channel = "stable"
components = ["rustfmt", "clippy"]
```

部分 CI job 依赖 **Nightly 工具链能力**（不是换 Edition 写法）：

| Job | 原因 | 覆盖方式 |
|-----|------|----------|
| `miri` | Miri 组件仅随 Nightly 分发 | `RUSTUP_TOOLCHAIN=nightly` |
| `public-api` | `cargo public-api` 需 rustdoc JSON | 同上 + `dtolnay/rust-toolchain@nightly` |

CI 里用环境变量 **覆盖** 本目录 `rust-toolchain.toml`，避免在 Stable 下误跑 miri / public-api。本地同理：

```bash
# 日常（尊重 rust-toolchain.toml → stable）
cargo test --workspace

# Miri / public-api（显式 nightly）
$env:RUSTUP_TOOLCHAIN = "nightly"   # PowerShell
cargo miri test -p item-16-miri

cd item-24-re-export/dep-lib
cargo public-api
```

## Item 25：`[workspace.dependencies]`

成员 crate 通过 `{ workspace = true }` 引用统一版本，例如 `item-04-error-types` 的 `anyhow` / `thiserror`。

### Dependabot + `cargo deny`

- [`.github/dependabot.yml`](../../../.github/dependabot.yml) — 每周更新 `01-ER/ER-demos` 等 lock
- 根目录 [`deny.toml`](../../../deny.toml) — advisories / licenses / bans
- CI `cargo-deny` job：`cargo deny check all`

本地：

```bash
cargo install cargo-deny
cargo deny check all
```

## Item 21：`cargo-semver-checks`

未发布 crate 用 **git baseline** 对比 PR 与 base 分支的 public API：

```bash
cd 01-ER/Chapter-04-Dependencies/Item-24-re-export-api-types/demo
cargo install cargo-semver-checks --locked
cargo semver-checks --package dep-lib --baseline-rev main
```

CI：`semver-checks` job（仅 `pull_request`），见 [item-24-re-export/README.md](./item-24-re-export/README.md)。

## Item 29：`clippy.toml`

本目录 [`clippy.toml`](./clippy.toml) 对 workspace 内 crate 生效。
