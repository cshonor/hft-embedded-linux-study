# 小节笔记与 Demo 规范（Network Programming with Rust）

与 [atomic/小节笔记与Demo规范.md](../01-atomic/小节笔记与Demo规范.md) 同一套约定，路径在 `rust_network_programming/stageNN_*/`。

---

## 目录结构（以 stage06 为范本）

| 文件 | 作用 |
|------|------|
| `本章学习笔记.md` | **章索引表**（书 § → 笔记 `.md` → demo 目录） |
| `notes/ChNN-*.md` | **全章贯通稿**（保留；按节拆分后在此文件顶部链到各节） |
| `X.Y-english-slug/X.Y-english-slug.md` | **该节正文**（标题用 `## X.Y …`，与书编号一致） |
| `X.Y-english-slug/code/*-demo.rs` | 可运行示例；由本章 `Cargo.toml` 的 `[[bin]]` 挂接 |

**唯一编号**：书 § = 文件名前缀 = 代码目录名。例如书 §6.3 → `6.3-introducing-reqwest.md` + `6.3-introducing-reqwest/`。

```
stage06_http_programming/
├── 本章学习笔记.md
├── notes/Ch06-…-学习笔记.md    ← 贯通稿（不删，只加链到各节）
├── 6.1-introducing-hyper/
│   ├── 6.1-introducing-hyper.md
│   └── code/
│       └── 6.1-introducing-hyper-raw-tcp-demo.rs
├── 6.3-introducing-reqwest/
│   ├── 6.3-introducing-reqwest.md
│   └── code/
│       └── 6.3-introducing-reqwest-get-demo.rs
├── Cargo.toml
└── README.md
```

---

## 扩展顺序建议

| 阶段 | 目录 | 书章 | 状态 |
|------|------|------|------|
| 范本 | `stage06_http_programming` | Ch6 | §6.1～6.4 笔记 + demo 已搭 |
| 阻塞 Socket | `stage03_std_tcp_udp` | Ch3 | §3.1～3.5 + demo 已搭 |
| Serde/解析 | `stage04_serde_parse` | Ch4 | §4.1～4.4 + demo 已搭 |
| 应用协议 | `stage05_app_protocol` | Ch5 | §5.1～5.4 + demo 已搭 |
| Tokio | `stage07_tokio_async_net` | Ch7 | §7.1～7.3 + demo 已搭 |
| 安全 | `stage08_network_security` | Ch8 | §8.1～8.3 + demo 已搭 |
| 附录 | `stage09_appendix_deep` | 附录 | §9.1～9.6 + demo 已搭 |
| 待拆 | `stage02`、`stage01` | Ch2、Ch1 | 贯通稿 + 索引 |

**禁止**：在未要求时改写 `notes/` 贯通稿正文；新增内容放在 `X.Y-slug.md` 与 demo 目录。

---

## 运行 demo

```bash
# Ch3
cargo run --manifest-path 05-Async-Concurrency-Network/03-rust_network_programming/stage03_std_tcp_udp/Cargo.toml --bin demo_3_1_tcp_echo_server

# Ch7
cargo run --manifest-path 05-Async-Concurrency-Network/03-rust_network_programming/stage07_tokio_async_net/Cargo.toml --bin demo_7_2_echo_server

# Ch8
cargo run --manifest-path 05-Async-Concurrency-Network/03-rust_network_programming/stage08_network_security/Cargo.toml --bin demo_8_2_ring_x25519

# Ch9
cargo run --manifest-path 05-Async-Concurrency-Network/03-rust_network_programming/stage09_appendix_deep/Cargo.toml --bin demo_9_4_pest
```

`6.2` Rocket 依赖 nightly/数据库栈，demo 目录暂留占位，笔记中说明本地按书配置即可。
