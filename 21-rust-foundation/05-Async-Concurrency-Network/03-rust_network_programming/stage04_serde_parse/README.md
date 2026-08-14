# Stage 04 — 序列化与协议解析

**对应书籍**：第 4 章（§4.1～4.4，含 §4.1.1）

## 结构

| 类型 | 路径 |
|------|------|
| 章索引 | [本章学习笔记.md](./本章学习笔记.md) |
| 按节笔记 | `4.1-serde-serialization.md` … `4.4-summary.md` |
| 贯通稿 | [notes/Ch04-…](./notes/Ch04-数据序列化反序列化与解析-学习笔记.md) |
| Demo | [Cargo.toml](./Cargo.toml) |

```bash
cargo run --manifest-path 05-Async-Concurrency-Network/03-rust_network_programming/stage04_serde_parse/Cargo.toml --bin demo_4_1_serde_point3d
cargo run --manifest-path 05-Async-Concurrency-Network/03-rust_network_programming/stage04_serde_parse/Cargo.toml --bin demo_4_2_nom_http_line
cargo run --manifest-path 05-Async-Concurrency-Network/03-rust_network_programming/stage04_serde_parse/Cargo.toml --bin demo_4_3_binary_length_prefix
```

与 [stage03](../stage03_std_tcp_udp/) TCP 分帧合并练习。
