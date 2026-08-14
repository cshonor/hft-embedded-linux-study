# Stage 03 — Rust 原生同步网络（std）

**对应书籍**：第 3 章 — *TCP and UDP Using Rust*（§3.1～3.5）

## 结构（与 atomic / stage06 一致）

| 类型 | 路径 |
|------|------|
| 章索引 | [本章学习笔记.md](./本章学习笔记.md) |
| 按节笔记 | `3.1-simple-tcp-server-client.md` … `3.5-summary.md` |
| 贯通稿 | [notes/Ch03-…](./notes/Ch03-使用Rust进行TCP和UDP编程-学习笔记.md) |
| Demo | [Cargo.toml](./Cargo.toml) · 仅 `std`，无第三方依赖 |

规范：[../小节笔记与Demo规范.md](../小节笔记与Demo规范.md)

## 运行 Demo

```bash
# 3.1 TCP（终端 A server，终端 B client）
cargo run --manifest-path 05-Async-Concurrency-Network/03-rust_network_programming/stage03_std_tcp_udp/Cargo.toml --bin demo_3_1_tcp_echo_server
cargo run --manifest-path 05-Async-Concurrency-Network/03-rust_network_programming/stage03_std_tcp_udp/Cargo.toml --bin demo_3_1_tcp_echo_client

# 3.2 UDP
cargo run --manifest-path 05-Async-Concurrency-Network/03-rust_network_programming/stage03_std_tcp_udp/Cargo.toml --bin demo_3_2_udp_echo_server
cargo run --manifest-path 05-Async-Concurrency-Network/03-rust_network_programming/stage03_std_tcp_udp/Cargo.toml --bin demo_3_2_udp_echo_client

# 3.2.1 多播（先 recv 再 send）
cargo run --manifest-path 05-Async-Concurrency-Network/03-rust_network_programming/stage03_std_tcp_udp/Cargo.toml --bin demo_3_2_1_multicast_recv
cargo run --manifest-path 05-Async-Concurrency-Network/03-rust_network_programming/stage03_std_tcp_udp/Cargo.toml --bin demo_3_2_1_multicast_send

cargo run --manifest-path 05-Async-Concurrency-Network/03-rust_network_programming/stage03_std_tcp_udp/Cargo.toml --bin demo_3_3_std_net_utilities
cargo run --manifest-path 05-Async-Concurrency-Network/03-rust_network_programming/stage03_std_tcp_udp/Cargo.toml --bin demo_3_5_summary
```

## 学习定位

**地基级** — 阻塞 Socket → [stage07](../stage07_tokio_async_net/) 异步。
