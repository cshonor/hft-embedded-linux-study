# Stage 07 — Tokio 异步网络（TOP1）

**对应书籍**：第 7 章（§7.1～7.3）· 对照 [`02-async_tokio/`](../../02-async_tokio/)

## 结构

| 类型 | 路径 |
|------|------|
| 章索引 | [本章学习笔记.md](./本章学习笔记.md) |
| 按节笔记 | `7.1-looking-into-the-future.md` … `7.3-conclusion.md` |
| 贯通稿 | [notes/Ch07-…](./notes/Ch07-使用Tokio进行异步网络编程-学习笔记.md) |
| Demo | [Cargo.toml](./Cargo.toml) |

```bash
cargo run --manifest-path 05-Async-Concurrency-Network/03-rust_network_programming/stage07_tokio_async_net/Cargo.toml --bin demo_7_1_join
cargo run --manifest-path 05-Async-Concurrency-Network/03-rust_network_programming/stage07_tokio_async_net/Cargo.toml --bin demo_7_2_echo_server
cargo run --manifest-path 05-Async-Concurrency-Network/03-rust_network_programming/stage07_tokio_async_net/Cargo.toml --bin demo_7_2_echo_client
```

前置 [stage03](../stage03_std_tcp_udp/) 阻塞 Socket。
