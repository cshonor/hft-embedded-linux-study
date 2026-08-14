# Stage 05 — 应用层协议

**对应书籍**：第 5 章（§5.1～5.4）

## 结构

| 类型 | 路径 |
|------|------|
| 章索引 | [本章学习笔记.md](./本章学习笔记.md) |
| 按节笔记 | `5.1-introduction-to-rpc.md` … `5.4-summary.md` |
| 贯通稿 | [notes/Ch05-…](./notes/Ch05-应用层协议-学习笔记.md) |
| Demo | [Cargo.toml](./Cargo.toml) |

```bash
cargo run --manifest-path 05-Async-Concurrency-Network/03-rust_network_programming/stage05_app_protocol/Cargo.toml --bin demo_5_1_rpc_server
cargo run --manifest-path 05-Async-Concurrency-Network/03-rust_network_programming/stage05_app_protocol/Cargo.toml --bin demo_5_1_rpc_client
cargo run --manifest-path 05-Async-Concurrency-Network/03-rust_network_programming/stage05_app_protocol/Cargo.toml --bin demo_5_2_smtp_build
```

**5.1** demo 为 JSON-RPC 概念演示；完整 **gRPC/tonic**、**rust-ftp** 按书另建项目。
