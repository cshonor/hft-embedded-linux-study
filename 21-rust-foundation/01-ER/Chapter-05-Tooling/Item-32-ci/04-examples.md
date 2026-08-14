# Item 32 · 案例与代码

← [Item 32 目录](./README.md)

### Build & Test 清单

| 检查 | 命令 / 做法 | 关联 |
|------|-------------|------|
| 全 feature | `cargo hack check --feature-powerset` | Item 26 |
| `no_std` 交叉编译 | `cargo build --target thumbv6m-none-eabi --no-default-features` | Item 33 |
| **MSRV** | `cargo +1.xx.0 check`（与 `rust-version` 一致） | Item 21 |
| 单元 + 集成 + doc | `cargo test` | Item 30 |
| Examples | `cargo test --examples` | Item 27 / 30 |
| 库无 lock 构建 | CI job 删/忽略 lock，测最新依赖 | Item 25 |
| codegen 一致 | 重跑 `prost-build` 等 → `git diff --exit-code` | — |

### Tooling 清单

```bash
cargo fmt --check
cargo clippy -- -Dwarnings          # Item 29
cargo doc --no-deps                 # Item 27 死链
cargo deny check                    # Item 25
cargo udeps                         # Item 25
cargo tarpaulin …                   # Item 30 覆盖率
```

### 自定义脚本示例

- 扫描 `TODO:` 无责任人
- 检测硬编码 secret / token
- `no_panic` crate 编译（Item 18，§6 其他 Item 曾提及）

### 参考 PR 本地复现

```bash
cargo fmt --check && cargo clippy -- -Dwarnings && cargo test --all-features
```

---
