# Item 24: 公开依赖 + `pub use rand`

## 方案 A：`pub use`（`dep-lib` + `consumer`）

`dep-lib` 在 API 里使用 `rand 0.8` 并 **重导出**；`consumer` 通过 `dep_lib::rand` 构造 RNG，类型一致。

### 错误用法（勿编译）

若 consumer 自己 `use rand`（且解析到 **另一 semver 版本**），传入 `pick_number_with` 会报 trait 不满足：

```text
the trait bound ThreadRng: rand_core::RngCore is not satisfied
```

## 方案 B：newtype 隐藏（`dep-lib-newtype` + `consumer-newtype`）

API **不暴露** `rand` 类型；内部封装 `Picker::pick(max)`。下游无需 `pub use rand`，也避免公开依赖 Semver 绑定。

```bash
cargo run -p consumer-newtype
```

## 诊断

```bash
cd 01-ER/Chapter-04-Dependencies/Item-24-re-export-api-types/demo
cargo tree -p consumer -d rand
```

## `cargo-public-api`（Item 24 / 31）

监控 pub API 是否意外泄漏依赖类型：

```bash
cargo install cargo-public-api
cd dep-lib
cargo public-api diff  # 需已有 baseline 或对比 git tag
cargo public-api          # 列出当前 public API（需 nightly + RUSTUP_TOOLCHAIN）
```

CI 可选步骤见 [er-study-ci.yml](../../../.github/workflows/er-study-ci.yml) `public-api` job（文档性 dump）。

## `cargo-semver-checks`（Item 21）

PR 上对比 `dep-lib` 与 base 分支的 semver 合规性（未发布 crate 用 `--baseline-rev`）：

```bash
cargo install cargo-semver-checks --locked
cargo semver-checks --package dep-lib --baseline-rev main
```

CI：`semver-checks` job（`pull_request` 触发）。

## 运行

```bash
cargo run -p consumer
cargo run -p consumer-newtype
```
