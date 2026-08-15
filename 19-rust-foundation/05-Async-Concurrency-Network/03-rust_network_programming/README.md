# Network Programming with Rust — 学习区

> 所属：[05-Async-Concurrency-Network](../README.md) · **03/03** · 前置 [`01-atomic/`](../01-atomic/README-学习区.md) + [`02-async_tokio/`](../02-async_tokio/README.md) · 建议 **stage03 阻塞 Socket** 后再 **stage07 + 02-async_tokio**

本目录为《Network Programming with Rust》的**结构化学习路线**，与同级的 `02-async_tokio/`、`01-atomic/` **互补**：侧重 **Socket 实战、IO 模型、协议与工程化网络**。

**笔记 + Demo 约定**（与 `01-atomic/` 相同：`X.Y-slug/X.Y-slug.md` + `code/`）：[小节笔记与Demo规范.md](./小节笔记与Demo规范.md) · 已拆节：**Ch3～Ch9**（`stage03`～`stage09`；Ch1～2 仍为贯通稿）。

## 阶段总览

| 阶段 | 目录 | 定位 |
|------|------|------|
| Stage 1 | `stage01_network_basic` | 书 Ch1 网络理论打底（[Ch01 笔记](stage01_network_basic/notes/Ch01-客户端服务器网络简介-学习笔记.md)） |
| Stage 2 | `stage02_rust_basis` | 书 Ch2 共 **10 节**（[索引](stage02_rust_basis/本章学习笔记.md) · [精读](stage02_rust_basis/notes/Ch02-Rust及其生态系统简介-学习笔记.md)） |
| Stage 3 | `stage03_std_tcp_udp` | 书 Ch3 **5 节 + demo**（[索引](stage03_std_tcp_udp/本章学习笔记.md) · [贯通稿](stage03_std_tcp_udp/notes/Ch03-使用Rust进行TCP和UDP编程-学习笔记.md)） |
| Stage 4 | `stage04_serde_parse` | 书 Ch4 **4 节 + demo**（[索引](stage04_serde_parse/本章学习笔记.md) · [贯通稿](stage04_serde_parse/notes/Ch04-数据序列化反序列化与解析-学习笔记.md)） |
| Stage 5 | `stage05_app_protocol` | 书 Ch5 **4 节 + demo**（[索引](stage05_app_protocol/本章学习笔记.md) · [贯通稿](stage05_app_protocol/notes/Ch05-应用层协议-学习笔记.md)） |
| Stage 6 | `stage06_http_programming` | 书 Ch6 **4 节 + demo**（[索引](stage06_http_programming/本章学习笔记.md) · [贯通稿](stage06_http_programming/notes/Ch06-在互联网上进行HTTP通信-学习笔记.md)） |
| Stage 7 | `stage07_tokio_async_net` | 书 Ch7 **3 主节 + demo** TOP1（[索引](stage07_tokio_async_net/本章学习笔记.md) · [贯通稿](stage07_tokio_async_net/notes/Ch07-使用Tokio进行异步网络编程-学习笔记.md) · [`02-async_tokio/`](../02-async_tokio/)） |
| Stage 8 | `stage08_network_security` | 书 Ch8 **3 主节 + demo**（[索引](stage08_network_security/本章学习笔记.md) · [贯通稿](stage08_network_security/notes/Ch08-安全-学习笔记.md)） |
| Stage 9 | `stage09_appendix_deep` | 书 **附录 6 主节 + demo**（[索引](stage09_appendix_deep/本章学习笔记.md) · [贯通稿](stage09_appendix_deep/notes/附录-扩展与替代方案-学习笔记.md)） |

## 推荐学习顺序（减少弯路）

1. `stage01_network_basic` — 概念打底  
2. `stage03_std_tcp_udp` — **先阻塞 Socket，再谈多路复用与异步**  
3. `stage07_tokio_async_net` — 与 `02-async_tokio/` 笔记、根 crate 示例对照  
4. `stage04_serde_parse` — 协议与数据形态  
5. `stage08_network_security` — 上线前再深挖  
6. 其余阶段按兴趣与时间补全  

## 与同级目录的关系

- **并发 / 原子 / 内存序**：见 [`../01-atomic/`](../01-atomic/README-学习区.md)（含 `rayon/` 子 crate）
- **异步 Rust / Tokio 概念**：见 [`../02-async_tokio/`](../02-async_tokio/README.md)
- **IR 对照（读完 network 后）**：见 [`05/04_Learn-LLVM-17`](../../06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/README.md)

## 本目录约定（可选）

- 每个 `stage*/` 内已有 `README.md`：章节对应、优先级、是否必写 Demo。  
- 若增加可运行代码：建议 `stageNN_*/demo/` 下独立小 crate，或后续再挂入根 workspace，避免与现有成员耦合过重。
