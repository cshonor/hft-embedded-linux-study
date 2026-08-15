# 第 13 章 · I/O 系统

> 所属：[03 DeepRustStdLib](../README.md) · 前：[第 12 章 文件系统](../chapter12_filesystem/README.md) · 后：[第 14 章 异步编程](../chapter14_async/README.md) · 原书目录：[本书目录 § 第 13 章](../本书目录.md#第-13-章--io-系统)

**本章定位**：**「一切皆文件 / 一切皆流」** — **Read/Write · readv/writev · Stdin/Stdout/Stderr 全局锁 · `std::net` Socket** · **[13.4/13.5 源码附录](./13.4-vectored-stdio-walkthrough.md)**；同步 IO 终点 · [Ch14 异步](../chapter14_async/README.md) 起点。

**原书主线**：13.1 trait+向量+stdin 锁 → 13.2 stdout/stderr 缓冲 → 13.3 网络同一 Read/Write。

**阅读顺序**：**[13.0 总览](./13.0-chapter-overview.md) → 13.1 → 13.2 → 13.3**

---


<!-- AUTO:SECTION-INDEX -->

| 节 | 主题 | 笔记 |
|:---:|------|------|
| **13.0** | 本章总览 · 三层架构 | [笔记](./13.0-chapter-overview.md) |
| **13.1** | 标准输入 Stdin 类型分析 | [笔记](./13.1-stdin-overview.md) |
| **13.1.1** | Read Trait | [笔记](./13.1.1-read-trait.md) |
| **13.1.2** | 向量读/写类型分析 | [笔记](./13.1.2-vec-read-write.md) |
| **13.1.3** | 对外接口层 | [笔记](./13.1.3-stdin-public-api.md) |
| **13.2** | 标准输出 Stdout 类型分析 | [笔记](./13.2-stdout.md) |
| **13.3** | 网络 I/O | [笔记](./13.3-network-io.md) |
| **13.4** | 向量 IO + stdio 走读（附录） | [笔记](./13.4-vectored-stdio-walkthrough.md) |
| **13.5** | Socket 走读（附录） | [笔记](./13.5-net-socket-walkthrough.md) |

<!-- /AUTO:SECTION-INDEX -->
## 子节索引

| 节 | 主题 | 笔记 |
|:---:|------|------|
| **13.0** | 本章总览 | ✅ |
| **13.1** | 标准输入 `Stdin` | ✅ |
| **13.1.1** | `Read` / `Write` Trait | ✅ |
| **13.1.2** | 向量读/写 | ✅ |
| **13.1.3** | Stdin 对外 API | ✅ |
| **13.2** | Stdout / Stderr | ✅ |
| **13.3** | 网络 I/O | ✅ |
| **13.4** | 向量 IO + stdio 走读 | ✅ |
| **13.5** | Socket 走读 | ✅ |

---

## 与主线对照

| 本章 | 本仓库延伸 |
|------|------------|
| `Read` / `Write` | [Ch9 · IO 基础](../chapter09_userspace_std_basics/README.md) |
| 管道 fd | [第 10 章](../chapter10_process_management/README.md) |
| 磁盘 File | [第 12 章](../chapter12_filesystem/README.md) |
| TCP/UDP 实战 | [05-rust_network stage03](../../05-Async-Concurrency-Network/03-rust_network_programming/stage03_std_tcp_udp/README.md) |
| async 网络 | [第 14 章](../chapter14_async/README.md) · [05-async](../../05-Async-Concurrency-Network/02-async/) |

---

## HFT 阅读提示

| 节 | 实盘关联 |
|----|----------|
| **13.1.1～13.1.2** | 协议帧 `read_exact` / `writev` 头+体 |
| **13.2** | 热路径避免 `println!` |
| **13.3** | 阻塞 `TcpStream` vs `tokio::net` |
