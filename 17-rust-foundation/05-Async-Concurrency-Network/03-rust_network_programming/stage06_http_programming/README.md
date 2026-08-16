# Stage 06 — HTTP 编程

**对应书籍**：第 6 章 — *Talking HTTP in the Internet*（§6.1～6.4）

## 结构（与 atomic 一致）

| 类型 | 路径 |
|------|------|
| 章索引 | [本章学习笔记.md](./本章学习笔记.md) |
| 按节笔记 | `6.1-introducing-hyper.md` … `6.4-summary.md` |
| 贯通稿 | [notes/Ch06-…](./notes/Ch06-在互联网上进行HTTP通信-学习笔记.md) |
| Demo | 章根 `Cargo.toml` + `6.Y-slug/`；**6.2** 另见 `6.2-introducing-rocket/Cargo.toml` |

规范：[../小节笔记与Demo规范.md](../小节笔记与Demo规范.md)

## 运行 Demo

```bash
# 6.1 本地 HTTP（另开终端 curl http://127.0.0.1:18080/）
cargo run --manifest-path 05-Async-Concurrency-Network/03-rust_network_programming/stage06_http_programming/Cargo.toml --bin demo_6_1_raw_http

# 6.2 Rocket（http://127.0.0.1:18082/ 、 /api/ping）
cargo run --manifest-path 05-Async-Concurrency-Network/03-rust_network_programming/stage06_http_programming/6.2-introducing-rocket/Cargo.toml --bin demo_6_2_rocket_hello

# 6.3 外网 GET
cargo run --manifest-path 05-Async-Concurrency-Network/03-rust_network_programming/stage06_http_programming/Cargo.toml --bin demo_6_3_reqwest_get

cargo run --manifest-path 05-Async-Concurrency-Network/03-rust_network_programming/stage06_http_programming/Cargo.toml --bin demo_6_4_summary
```

## 学习定位

调 API 优先 **§6.3 reqwest**；服务侧与 [Ch07](../stage07_tokio_async_net/) Tokio 对照学习。
