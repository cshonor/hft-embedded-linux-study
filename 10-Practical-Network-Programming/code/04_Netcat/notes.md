# 04 · Netcat（nc）

<a id="pnp-04-goal"></a>

## 目标

最小 **双向字节管道**：stdin ↔ TCP ↔ 对端；理解半关闭、`shutdown`、两端同时读写。

<a id="pnp-04-unp"></a>

## UNP 对照

- Ch5 并发 echo、Ch6 I/O 多路复用、Ch14 高级 I/O

<a id="pnp-04-run"></a>

## Rust 运行

```bash
cd PNP/code/04_Netcat/rewrite_rust

# 监听（单连接）
cargo run -- -l 9000

# 另一终端连接
cargo run -- 127.0.0.1 9000
```

<a id="pnp-04-pitfalls"></a>

## 坑点

- 只关写端 vs `close` 整连接（对端 `read` 得 0）
- 单线程阻塞：一端堵住另一端（课程常引到非阻塞 / epoll）
- Windows 与 POSIX 行为差异（本 Rust 版用 `std::net`，跨平台）
