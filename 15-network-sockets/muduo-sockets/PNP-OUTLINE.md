# PNP 实验大纲（常见模块）

> 集号因课程版本而异；听课时在 [study.md](./study.md) 补「第 N 集 → 实验目录」。

| 序号 | 目录 | 主题 / 坑点 | UNP 对照 |
|------|------|-------------|----------|
| 01 | [01_SocketBasics](./code/01_SocketBasics/notes.md) | 地址、字节序、Daytime | Ch1 |
| 02 | [02_TCPByteStream_粘包](./code/02_TCPByteStream_粘包/notes.md) | 无消息边界、`read` 循环 | Ch2–3 |
| 03 | [03_SelfConnect_自连接](./code/03_SelfConnect_自连接/notes.md) | `listenfd`/`connfd`、自连接 | Ch1.5、Ch4 |
| 04 | [04_Netcat](./code/04_Netcat/notes.md) | 双向转发、半关闭 | Ch5–6、Ch14 |
| 05 | [05_TTCP](./code/05_TTCP/notes.md) | 吞吐、`readn`/`writen` | Ch3.9、Ch14 |
| 06 | [06_NonBlockingIO](./code/06_NonBlockingIO/notes.md) | `EAGAIN`、`EINTR` | Ch16 |
| 07 | [07_IO复用_epoll](./code/07_IO复用_epoll/notes.md) | select/poll/epoll | Ch6、Ch16 |
| 08 | [08_UDP_Multicast](./code/08_UDP_Multicast/notes.md) | UDP、组播 | Ch8、Ch21–22 |
| 09 | [09_Serialization陷阱](./code/09_Serialization陷阱/notes.md) | Protobuf 对齐、版本 | PNP 独有 |

**已实现 Rust**：04 Netcat（最小可用）。其余目录为占位，听课后补 `original_cpp` 与笔记。
