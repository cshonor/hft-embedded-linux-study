# 14.3 HFT Workspace demo

笔记：[14.3-Cargo工作空间.md](../14.3-Cargo工作空间.md)

```
hft-workspace-demo/
├ Cargo.toml          [workspace]
├ engine/             lib
├ market_data/        lib
├ gateway/            bin（调 engine + market_data）
└ tools/              src/bin/replay.rs
```

```bash
cargo build
cargo run -p gateway
cargo run -p tools --bin replay
cargo test
```
